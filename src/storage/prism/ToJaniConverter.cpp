#include "src/storage/prism/ToJaniConverter.h"

#include "src/storage/expressions/ExpressionManager.h"

#include "src/storage/prism/Program.h"
#include "src/storage/prism/CompositionToJaniVisitor.h"
#include "src/storage/jani/Model.h"

#include "src/utility/macros.h"
#include "src/exceptions/NotImplementedException.h"

namespace storm {
    namespace prism {
        
        storm::jani::Model ToJaniConverter::convert(storm::prism::Program const& program, bool allVariablesGlobal) const {
            std::shared_ptr<storm::expressions::ExpressionManager> manager = program.getManager().getSharedPointer();
            
            // Start by creating an empty JANI model.
            storm::jani::ModelType modelType;
            switch (program.getModelType()) {
                case Program::ModelType::DTMC: modelType = storm::jani::ModelType::DTMC;
                    break;
                case Program::ModelType::CTMC: modelType = storm::jani::ModelType::CTMC;
                    break;
                case Program::ModelType::MDP: modelType = storm::jani::ModelType::MDP;
                    break;
                case Program::ModelType::CTMDP: modelType = storm::jani::ModelType::CTMDP;
                    break;
                case Program::ModelType::MA: modelType = storm::jani::ModelType::MA;
                    break;
                default: modelType = storm::jani::ModelType::UNDEFINED;
            }
            storm::jani::Model janiModel("jani_from_prism", modelType, 1, manager);
            
            // Add all constants of the PRISM program to the JANI model.
            for (auto const& constant : program.getConstants()) {
                janiModel.addConstant(storm::jani::Constant(constant.getName(), constant.getExpressionVariable(), constant.isDefined() ? boost::optional<storm::expressions::Expression>(constant.getExpression()) : boost::none));
            }
            
            // Maintain a mapping from expression variables to JANI variables so we can fill in the correct objects when
            // creating assignments.
            std::map<storm::expressions::Variable, std::reference_wrapper<storm::jani::Variable const>> variableToVariableMap;
            
            // Add all global variables of the PRISM program to the JANI model.
            for (auto const& variable : program.getGlobalIntegerVariables()) {
                if (variable.hasInitialValue()) {
                    storm::jani::BoundedIntegerVariable const& createdVariable = janiModel.addVariable(storm::jani::BoundedIntegerVariable(variable.getName(), variable.getExpressionVariable(), variable.getInitialValueExpression(), false, variable.getLowerBoundExpression(), variable.getUpperBoundExpression()));
                    variableToVariableMap.emplace(variable.getExpressionVariable(), createdVariable);
                } else {
                    storm::jani::BoundedIntegerVariable const& createdVariable = janiModel.addVariable(storm::jani::BoundedIntegerVariable(variable.getName(), variable.getExpressionVariable(), false, variable.getLowerBoundExpression(), variable.getUpperBoundExpression()));
                    variableToVariableMap.emplace(variable.getExpressionVariable(), createdVariable);
                }
            }
            for (auto const& variable : program.getGlobalBooleanVariables()) {
                if (variable.hasInitialValue()) {
                    storm::jani::BooleanVariable const& createdVariable = janiModel.addVariable(storm::jani::BooleanVariable(variable.getName(), variable.getExpressionVariable(), variable.getInitialValueExpression(), false));
                    variableToVariableMap.emplace(variable.getExpressionVariable(), createdVariable);
                } else {
                    storm::jani::BooleanVariable const& createdVariable = janiModel.addVariable(storm::jani::BooleanVariable(variable.getName(), variable.getExpressionVariable(), false));
                    variableToVariableMap.emplace(variable.getExpressionVariable(), createdVariable);
                }
            }
            
            // Add all actions of the PRISM program to the JANI model.
            for (auto const& action : program.getActions()) {
                // Ignore the empty action as every JANI program has predefined tau action.
                if (!action.empty()) {
                    janiModel.addAction(storm::jani::Action(action));
                }
            }
            
            // Because of the rules of JANI, we have to make all variables of modules global that are read by other modules.
            
            // Create a mapping from variables to the indices of module indices that write/read the variable.
            std::map<storm::expressions::Variable, std::set<uint_fast64_t>> variablesToAccessingModuleIndices;
            for (uint_fast64_t index = 0; index < program.getNumberOfModules(); ++index) {
                storm::prism::Module const& module = program.getModule(index);
                
                for (auto const& command : module.getCommands()) {
                    std::set<storm::expressions::Variable> variables = command.getGuardExpression().getVariables();
                    for (auto const& variable : variables) {
                        variablesToAccessingModuleIndices[variable].insert(index);
                    }
                    
                    for (auto const& update : command.getUpdates()) {
                        for (auto const& assignment : update.getAssignments()) {
                            variables = assignment.getExpression().getVariables();
                            for (auto const& variable : variables) {
                                variablesToAccessingModuleIndices[variable].insert(index);
                            }
                            variablesToAccessingModuleIndices[assignment.getVariable()].insert(index);
                        }
                    }
                }
            }
            
            // Go through the reward models and construct assignments to the transient variables that are to be added to
            // edges and transient assignments that are added to the locations.
            std::map<uint_fast64_t, std::vector<storm::jani::Assignment>> transientEdgeAssignments;
            std::vector<storm::jani::Assignment> transientLocationAssignments;
            for (auto const& rewardModel : program.getRewardModels()) {
                auto newExpressionVariable = manager->declareRationalVariable(rewardModel.getName().empty() ? "default" : rewardModel.getName());
                storm::jani::RealVariable const& newTransientVariable = janiModel.addVariable(storm::jani::RealVariable(rewardModel.getName(), newExpressionVariable, true));
                
                if (rewardModel.hasStateRewards()) {
                    storm::expressions::Expression transientLocationExpression;
                    for (auto const& stateReward : rewardModel.getStateRewards()) {
                        storm::expressions::Expression rewardTerm = stateReward.getStatePredicateExpression().isTrue() ? stateReward.getRewardValueExpression() : storm::expressions::ite(stateReward.getStatePredicateExpression(), stateReward.getRewardValueExpression(), manager->rational(0));
                        if (transientLocationExpression.isInitialized()) {
                            transientLocationExpression = transientLocationExpression + rewardTerm;
                        } else {
                            transientLocationExpression = rewardTerm;
                        }
                    }
                    transientLocationAssignments.emplace_back(newTransientVariable, transientLocationExpression);
                }
                
                std::map<uint_fast64_t, storm::expressions::Expression> actionIndexToExpression;
                for (auto const& actionReward : rewardModel.getStateActionRewards()) {
                    storm::expressions::Expression rewardTerm = actionReward.getStatePredicateExpression().isTrue() ? actionReward.getRewardValueExpression() : storm::expressions::ite(actionReward.getStatePredicateExpression(), actionReward.getRewardValueExpression(), manager->rational(0));
                    auto it = actionIndexToExpression.find(janiModel.getActionIndex(actionReward.getActionName()));
                    if (it != actionIndexToExpression.end()) {
                        it->second = it->second + rewardTerm;
                    } else {
                        actionIndexToExpression[janiModel.getActionIndex(actionReward.getActionName())] = rewardTerm;
                    }
                }
                
                for (auto const& entry : actionIndexToExpression) {
                    auto it = transientEdgeAssignments.find(entry.first);
                    if (it != transientEdgeAssignments.end()) {
                        it->second.push_back(storm::jani::Assignment(newTransientVariable, entry.second));
                    } else {
                        std::vector<storm::jani::Assignment> assignments = {storm::jani::Assignment(newTransientVariable, entry.second)};
                        transientEdgeAssignments.emplace(entry.first, assignments);
                    }
                }
                STORM_LOG_THROW(!rewardModel.hasTransitionRewards(), storm::exceptions::NotImplementedException, "Transition reward translation currently not implemented.");
            }
            STORM_LOG_THROW(transientEdgeAssignments.empty() || transientLocationAssignments.empty() || !program.specifiesSystemComposition(), storm::exceptions::NotImplementedException, "Cannot translate reward models from PRISM to JANI that specify a custom system composition.");
            
            // Now create the separate JANI automata from the modules of the PRISM program. While doing so, we use the
            // previously built mapping to make variables global that are read by more than one module.
            bool firstModule = true;
            for (auto const& module : program.getModules()) {
                // Keep track of the action indices contained in this module.
                std::set<uint_fast64_t> actionIndicesOfModule;

                storm::jani::Automaton automaton(module.getName());
                for (auto const& variable : module.getIntegerVariables()) {
                    storm::jani::BoundedIntegerVariable newIntegerVariable = *storm::jani::makeBoundedIntegerVariable(variable.getName(), variable.getExpressionVariable(), variable.hasInitialValue() ? boost::make_optional(variable.getInitialValueExpression()) : boost::none, false, variable.getLowerBoundExpression(), variable.getUpperBoundExpression());
                    std::set<uint_fast64_t> const& accessingModuleIndices = variablesToAccessingModuleIndices[variable.getExpressionVariable()];
                    // If there is exactly one module reading and writing the variable, we can make the variable local to this module.
                    if (!allVariablesGlobal && accessingModuleIndices.size() == 1) {
                        storm::jani::BoundedIntegerVariable const& createdVariable = automaton.addVariable(newIntegerVariable);
                        variableToVariableMap.emplace(variable.getExpressionVariable(), createdVariable);
                    } else if (!accessingModuleIndices.empty()) {
                        // Otherwise, we need to make it global.
                        storm::jani::BoundedIntegerVariable const& createdVariable = janiModel.addVariable(newIntegerVariable);
                        variableToVariableMap.emplace(variable.getExpressionVariable(), createdVariable);
                    }
                }
                for (auto const& variable : module.getBooleanVariables()) {
                    storm::jani::BooleanVariable newBooleanVariable = *storm::jani::makeBooleanVariable(variable.getName(), variable.getExpressionVariable(), variable.hasInitialValue() ? boost::make_optional(variable.getInitialValueExpression()) : boost::none, false);
                    std::set<uint_fast64_t> const& accessingModuleIndices = variablesToAccessingModuleIndices[variable.getExpressionVariable()];
                    // If there is exactly one module reading and writing the variable, we can make the variable local to this module.
                    if (!allVariablesGlobal && accessingModuleIndices.size() == 1) {
                        storm::jani::BooleanVariable const& createdVariable = automaton.addVariable(newBooleanVariable);
                        variableToVariableMap.emplace(variable.getExpressionVariable(), createdVariable);
                    } else if (!accessingModuleIndices.empty()) {
                        // Otherwise, we need to make it global.
                        storm::jani::BooleanVariable const& createdVariable = janiModel.addVariable(newBooleanVariable);
                        variableToVariableMap.emplace(variable.getExpressionVariable(), createdVariable);
                    }
                }
                automaton.setInitialStatesRestriction(manager->boolean(true));
                
                // Create a single location that will have all the edges.
                uint64_t onlyLocationIndex = automaton.addLocation(storm::jani::Location("l"));
                automaton.addInitialLocation(onlyLocationIndex);
                
                // If we are translating the first module, we need to add the transient assignments to the location.
                if (firstModule) {
                    storm::jani::Location& onlyLocation = automaton.getLocation(onlyLocationIndex);
                    for (auto const& assignment : transientLocationAssignments) {
                        onlyLocation.addTransientAssignment(assignment);
                    }
                }
                
                for (auto const& command : module.getCommands()) {
                    actionIndicesOfModule.insert(command.getActionIndex());
                    
                    boost::optional<storm::expressions::Expression> rateExpression;
                    std::vector<storm::jani::EdgeDestination> destinations;
                    if (program.getModelType() == Program::ModelType::CTMC || program.getModelType() == Program::ModelType::CTMDP) {
                        for (auto const& update : command.getUpdates()) {
                            if (rateExpression) {
                                rateExpression = rateExpression.get() + update.getLikelihoodExpression();
                            } else {
                                rateExpression = update.getLikelihoodExpression();
                            }
                        }
                    }
                    
                    for (auto const& update : command.getUpdates()) {
                        std::vector<storm::jani::Assignment> assignments;
                        for (auto const& assignment : update.getAssignments()) {
                            assignments.push_back(storm::jani::Assignment(variableToVariableMap.at(assignment.getVariable()).get(), assignment.getExpression()));
                        }
                        
                        if (rateExpression) {
                            destinations.push_back(storm::jani::EdgeDestination(onlyLocationIndex, manager->integer(1) / rateExpression.get(), assignments));
                        } else {
                            destinations.push_back(storm::jani::EdgeDestination(onlyLocationIndex, update.getLikelihoodExpression(), assignments));
                        }
                    }
                    
                    // Create the edge object so we can add transient assignments.
                    storm::jani::Edge newEdge(onlyLocationIndex, janiModel.getActionIndex(command.getActionName()), rateExpression, command.getGuardExpression(), destinations);
                    
                    // Then add the transient assignments for the rewards.
                    auto transientEdgeAssignmentsToAdd = transientEdgeAssignments.find(janiModel.getActionIndex(command.getActionName()));
                    if (transientEdgeAssignmentsToAdd != transientEdgeAssignments.end()) {
                        for (auto const& assignment : transientEdgeAssignmentsToAdd->second) {
                            newEdge.addTransientAssignment(assignment);
                        }
                    }
                    
                    // Finally add the constructed edge.
                    automaton.addEdge(newEdge);
                }
                
                // Now remove for all actions of this module the corresponding transient assignments, because we must
                // not deal out this reward multiple times.
                // NOTE: This only works for the standard composition and not for any custom compositions. This case
                // must be checked for earlier.
                for (auto actionIndex : actionIndicesOfModule) {
                    auto it = transientEdgeAssignments.find(actionIndex);
                    if (it != transientEdgeAssignments.end()) {
                        transientEdgeAssignments.erase(it);
                    }
                }
                
                janiModel.addAutomaton(automaton);
                firstModule = false;
            }
            
            // Create an initial state restriction if there was an initial construct in the program.
            if (program.hasInitialConstruct()) {
                janiModel.setInitialStatesRestriction(program.getInitialConstruct().getInitialStatesExpression());
            } else {
                janiModel.setInitialStatesRestriction(manager->boolean(true));
            }
            
            // Set the standard system composition. This is possible, because we reject non-standard compositions anyway.
            if (program.specifiesSystemComposition()) {
                CompositionToJaniVisitor visitor;
                janiModel.setSystemComposition(visitor.toJani(program.getSystemCompositionConstruct().getSystemComposition(), janiModel));
            } else {
                janiModel.setSystemComposition(janiModel.getStandardSystemComposition());
            }
            
            janiModel.finalize();
            
            return janiModel;
        }
        
    }
}