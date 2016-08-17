#include "src/settings/modules/IOSettings.h"

#include "src/settings/SettingsManager.h"
#include "src/settings/SettingMemento.h"
#include "src/settings/Option.h"
#include "src/settings/OptionBuilder.h"
#include "src/settings/ArgumentBuilder.h"
#include "src/settings/Argument.h"
#include "src/exceptions/InvalidSettingsException.h"

namespace storm {
    namespace settings {
        namespace modules {
            
            const std::string IOSettings::moduleName = "io";
            const std::string IOSettings::exportDotOptionName = "exportdot";
            const std::string IOSettings::exportMatOptionName = "exportmat";
            const std::string IOSettings::explicitOptionName = "explicit";
            const std::string IOSettings::explicitOptionShortName = "exp";
            const std::string IOSettings::symbolicOptionName = "symbolic";
            const std::string IOSettings::symbolicOptionShortName = "s";
            const std::string IOSettings::explorationOrderOptionName = "explorder";
            const std::string IOSettings::explorationOrderOptionShortName = "eo";
            const std::string IOSettings::transitionRewardsOptionName = "transrew";
            const std::string IOSettings::stateRewardsOptionName = "staterew";
            const std::string IOSettings::choiceLabelingOptionName = "choicelab";
            const std::string IOSettings::constantsOptionName = "constants";
            const std::string IOSettings::constantsOptionShortName = "const";
            const std::string IOSettings::prismCompatibilityOptionName = "prismcompat";
            const std::string IOSettings::prismCompatibilityOptionShortName = "pc";
            
            IOSettings::IOSettings() : ModuleSettings(moduleName) {
                this->addOption(storm::settings::OptionBuilder(moduleName, prismCompatibilityOptionName, false, "Enables PRISM compatibility. This may be necessary to process some PRISM models.").setShortName(prismCompatibilityOptionShortName).build());
                this->addOption(storm::settings::OptionBuilder(moduleName, exportDotOptionName, "", "If given, the loaded model will be written to the specified file in the dot format.")
                                .addArgument(storm::settings::ArgumentBuilder::createStringArgument("filename", "The name of the file to which the model is to be written.").build()).build());
                this->addOption(storm::settings::OptionBuilder(moduleName, exportMatOptionName, "", "If given, the loaded model will be written to the specified file in the mat format.")
                                .addArgument(storm::settings::ArgumentBuilder::createStringArgument("filename", "the name of the file to which the model is to be writen.").build()).build());
                this->addOption(storm::settings::OptionBuilder(moduleName, explicitOptionName, false, "Parses the model given in an explicit (sparse) representation.").setShortName(explicitOptionShortName)
                                .addArgument(storm::settings::ArgumentBuilder::createStringArgument("transition filename", "The name of the file from which to read the transitions.").addValidationFunctionString(storm::settings::ArgumentValidators::existingReadableFileValidator()).build())
                                .addArgument(storm::settings::ArgumentBuilder::createStringArgument("labeling filename", "The name of the file from which to read the state labeling.").addValidationFunctionString(storm::settings::ArgumentValidators::existingReadableFileValidator()).build()).build());
                this->addOption(storm::settings::OptionBuilder(moduleName, symbolicOptionName, false, "Parses the model given in a symbolic representation.").setShortName(symbolicOptionShortName)
                                .addArgument(storm::settings::ArgumentBuilder::createStringArgument("filename", "The name of the file from which to read the symbolic model.").addValidationFunctionString(storm::settings::ArgumentValidators::existingReadableFileValidator()).build()).build());

                std::vector<std::string> explorationOrders = {"dfs", "bfs"};
                this->addOption(storm::settings::OptionBuilder(moduleName, explorationOrderOptionName, false, "Sets which exploration order to use.").setShortName(explorationOrderOptionShortName)
                                .addArgument(storm::settings::ArgumentBuilder::createStringArgument("name", "The name of the exploration order to choose. Available are: dfs and bfs.").addValidationFunctionString(storm::settings::ArgumentValidators::stringInListValidator(explorationOrders)).setDefaultValueString("bfs").build()).build());

                this->addOption(storm::settings::OptionBuilder(moduleName, transitionRewardsOptionName, false, "If given, the transition rewards are read from this file and added to the explicit model. Note that this requires the model to be given as an explicit model (i.e., via --" + explicitOptionName + ").")
                                .addArgument(storm::settings::ArgumentBuilder::createStringArgument("filename", "The file from which to read the transition rewards.").addValidationFunctionString(storm::settings::ArgumentValidators::existingReadableFileValidator()).build()).build());
                this->addOption(storm::settings::OptionBuilder(moduleName, stateRewardsOptionName, false, "If given, the state rewards are read from this file and added to the explicit model. Note that this requires the model to be given as an explicit model (i.e., via --" + explicitOptionName + ").")
                                .addArgument(storm::settings::ArgumentBuilder::createStringArgument("filename", "The file from which to read the state rewards.").addValidationFunctionString(storm::settings::ArgumentValidators::existingReadableFileValidator()).build()).build());
                this->addOption(storm::settings::OptionBuilder(moduleName, choiceLabelingOptionName, false, "If given, the choice labels are read from this file and added to the explicit model. Note that this requires the model to be given as an explicit model (i.e., via --" + explicitOptionName + ").")
                                .addArgument(storm::settings::ArgumentBuilder::createStringArgument("filename", "The file from which to read the choice labels.").addValidationFunctionString(storm::settings::ArgumentValidators::existingReadableFileValidator()).build()).build());
                this->addOption(storm::settings::OptionBuilder(moduleName, constantsOptionName, false, "Specifies the constant replacements to use in symbolic models. Note that Note that this requires the model to be given as an symbolic model (i.e., via --" + symbolicOptionName + ").").setShortName(constantsOptionShortName)
                                .addArgument(storm::settings::ArgumentBuilder::createStringArgument("values", "A comma separated list of constants and their value, e.g. a=1,b=2,c=3.").setDefaultValueString("").build()).build());
            }

            bool IOSettings::isExportDotSet() const {
                return this->getOption(exportDotOptionName).getHasOptionBeenSet();
            }
            
            std::string IOSettings::getExportDotFilename() const {
                return this->getOption(exportDotOptionName).getArgumentByName("filename").getValueAsString();
            }

            bool IOSettings::isExplicitSet() const {
                return this->getOption(explicitOptionName).getHasOptionBeenSet();
            }
            
            std::string IOSettings::getTransitionFilename() const {
                return this->getOption(explicitOptionName).getArgumentByName("transition filename").getValueAsString();
            }
                        
            std::string IOSettings::getLabelingFilename() const {
                return this->getOption(explicitOptionName).getArgumentByName("labeling filename").getValueAsString();
            }
            
            bool IOSettings::isSymbolicSet() const {
                return this->getOption(symbolicOptionName).getHasOptionBeenSet();
            }
            
            std::string IOSettings::getSymbolicModelFilename() const {
                return this->getOption(symbolicOptionName).getArgumentByName("filename").getValueAsString();
            }
            
            bool IOSettings::isExplorationOrderSet() const {
                return this->getOption(explorationOrderOptionName).getHasOptionBeenSet();
            }
            
            storm::builder::ExplorationOrder IOSettings::getExplorationOrder() const {
                std::string explorationOrderAsString = this->getOption(explorationOrderOptionName).getArgumentByName("name").getValueAsString();
                if (explorationOrderAsString == "dfs") {
                    return storm::builder::ExplorationOrder::Dfs;
                } else if (explorationOrderAsString == "bfs") {
                    return storm::builder::ExplorationOrder::Bfs;
                }
                STORM_LOG_THROW(false, storm::exceptions::IllegalArgumentValueException, "Unknown exploration order '" << explorationOrderAsString << "'.");
            }
            
            bool IOSettings::isTransitionRewardsSet() const {
                return this->getOption(transitionRewardsOptionName).getHasOptionBeenSet();
            }
            
            std::string IOSettings::getTransitionRewardsFilename() const {
                return this->getOption(transitionRewardsOptionName).getArgumentByName("filename").getValueAsString();
            }
            
            bool IOSettings::isStateRewardsSet() const {
                return this->getOption(stateRewardsOptionName).getHasOptionBeenSet();
            }
            
            std::string IOSettings::getStateRewardsFilename() const {
                return this->getOption(stateRewardsOptionName).getArgumentByName("filename").getValueAsString();
            }
            
            bool IOSettings::isChoiceLabelingSet() const {
                return this->getOption(choiceLabelingOptionName).getHasOptionBeenSet();
            }
                
            std::string IOSettings::getChoiceLabelingFilename() const {
                return this->getOption(choiceLabelingOptionName).getArgumentByName("filename").getValueAsString();
            }
            
            std::unique_ptr<storm::settings::SettingMemento> IOSettings::overridePrismCompatibilityMode(bool stateToSet) {
                return this->overrideOption(prismCompatibilityOptionName, stateToSet);
            }
            
            bool IOSettings::isConstantsSet() const {
                return this->getOption(constantsOptionName).getHasOptionBeenSet();
            }
            
            std::string IOSettings::getConstantDefinitionString() const {
                return this->getOption(constantsOptionName).getArgumentByName("values").getValueAsString();
            }
            
            bool IOSettings::isPrismCompatibilityEnabled() const {
                return this->getOption(prismCompatibilityOptionName).getHasOptionBeenSet();
            }

			void IOSettings::finalize() {
            }

            bool IOSettings::check() const {
                // Ensure that the model was given either symbolically or explicitly.
                STORM_LOG_THROW(!isSymbolicSet() || !isExplicitSet(), storm::exceptions::InvalidSettingsException, "The model may be either given in an explicit or a symbolic format, but not both.");
                return true;
            }

        } // namespace modules
    } // namespace settings
} // namespace storm