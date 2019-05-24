#include "DftTransformator.h"
#include "storm/exceptions/NotImplementedException.h"

namespace storm {
    namespace transformations {
        namespace dft {
            template<typename ValueType>
            DftTransformator<ValueType>::DftTransformator() {
            }

            template<typename ValueType>
            std::shared_ptr<storm::storage::DFT<ValueType>>
            DftTransformator<ValueType>::transformUniqueFailedBe(storm::storage::DFT<ValueType> const &dft) {
                storm::builder::DFTBuilder<ValueType> builder = storm::builder::DFTBuilder<ValueType>(true);
                // NOTE: if probabilities for constant BEs are introduced, change this to vector of tuples (name, prob)
                std::vector<std::string> failedBEs;

                for (size_t i = 0; i < dft.nrElements(); ++i) {
                    std::shared_ptr<storm::storage::DFTElement<ValueType> const> element = dft.getElement(i);
                    switch (element->type()) {
                        case storm::storage::DFTElementType::BE_EXP: {
                            STORM_LOG_DEBUG("Transform " + element->name() + " [BE (exp)]");
                            auto be_exp = std::static_pointer_cast<storm::storage::BEExponential<ValueType> const>(
                                    element);
                            builder.addBasicElementExponential(be_exp->name(), be_exp->activeFailureRate(),
                                                               be_exp->dormancyFactor());
                            break;
                        }
                        case storm::storage::DFTElementType::BE_CONST: {
                            auto be_const = std::static_pointer_cast<storm::storage::BEExponential<ValueType> const>(
                                    element);
                            if (be_const->canFail()) {
                                STORM_LOG_DEBUG("Transform " + element->name() + " [BE (const failed)]");
                                failedBEs.push_back(be_const->name());
                            } else {
                                STORM_LOG_DEBUG("Transform " + element->name() + " [BE (const failsafe)]");
                            }
                            // All original constant BEs are set to failsafe, failed BEs are later triggered by a new element
                            builder.addBasicElementConst(be_const->name(), false);
                            break;
                        }
                        case storm::storage::DFTElementType::AND:
                            STORM_LOG_DEBUG("Transform " + element->name() + " [AND]");
                            builder.addAndElement(element->name(), getChildrenVector(element));
                            break;
                        case storm::storage::DFTElementType::OR:
                            STORM_LOG_DEBUG("Transform " + element->name() + " [OR]");
                            builder.addOrElement(element->name(), getChildrenVector(element));
                            break;
                        case storm::storage::DFTElementType::VOT: {
                            STORM_LOG_DEBUG("Transform " + element->name() + " [VOT]");
                            auto vot = std::static_pointer_cast<storm::storage::DFTVot<ValueType> const>(element);
                            builder.addVotElement(vot->name(), vot->threshold(), getChildrenVector(vot));
                            break;
                        }
                        case storm::storage::DFTElementType::PAND: {
                            STORM_LOG_DEBUG("Transform " + element->name() + " [PAND]");
                            auto pand = std::static_pointer_cast<storm::storage::DFTPand<ValueType> const>(element);
                            builder.addPandElement(pand->name(), getChildrenVector(pand), pand->isInclusive());
                            break;
                        }
                        case storm::storage::DFTElementType::POR: {
                            STORM_LOG_DEBUG("Transform " + element->name() + " [POR]");
                            auto por = std::static_pointer_cast<storm::storage::DFTPor<ValueType> const>(element);
                            builder.addPandElement(por->name(), getChildrenVector(por), por->isInclusive());
                            break;
                        }
                        case storm::storage::DFTElementType::SPARE:
                            STORM_LOG_DEBUG("Transform " + element->name() + " [SPARE]");
                            builder.addSpareElement(element->name(), getChildrenVector(element));
                            break;
                        case storm::storage::DFTElementType::PDEP: {
                            auto dep = std::static_pointer_cast<storm::storage::DFTDependency<ValueType> const>(
                                    element);
                            if (dep->isFDEP()) {
                                STORM_LOG_DEBUG("Transform " + element->name() + " [FDEP]");
                            } else {
                                STORM_LOG_DEBUG("Transform " + element->name() + " [PDEP]");
                            }
                            builder.addDepElement(dep->name(), getChildrenVector(dep), dep->probability());
                            break;
                        }
                        case storm::storage::DFTElementType::SEQ:
                            STORM_LOG_DEBUG("Transform " + element->name() + " [SEQ]");
                            builder.addSequenceEnforcer(element->name(), getChildrenVector(element));
                            break;
                        case storm::storage::DFTElementType::MUTEX:
                            STORM_LOG_DEBUG("Transform " + element->name() + " [MUTEX]");
                            builder.addMutex(element->name(), getChildrenVector(element));
                            break;
                        default:
                            STORM_LOG_THROW(false, storm::exceptions::NotImplementedException,
                                            "DFT type '" << element->type() << "' not known.");
                            break;
                    }

                }
                // At this point the DFT is an exact copy of the original, except for all constant failure probabilities being 0
                if (!failedBEs.empty()) {
                    builder.addBasicElementConst("Unique_Constant_Failure", true);
                    failedBEs.insert(std::begin(failedBEs), "Unique_Constant_Failure");
                    builder.addDepElement("Failure_Trigger", failedBEs, storm::utility::one<ValueType>());
                }

                builder.setTopLevel(dft.getTopLevelGate()->name());

                STORM_LOG_DEBUG("Transformation complete!");
                return std::make_shared<storm::storage::DFT<ValueType>>(builder.build());
            }

            template<typename ValueType>
            std::shared_ptr<storm::storage::DFT<ValueType>>
            DftTransformator<ValueType>::transformBinaryFDEPs(storm::storage::DFT<ValueType> const &dft) {
                storm::builder::DFTBuilder<ValueType> builder = storm::builder::DFTBuilder<ValueType>(true);

                for (size_t i = 0; i < dft.nrElements(); ++i) {
                    std::shared_ptr<storm::storage::DFTElement<ValueType> const> element = dft.getElement(i);
                    switch (element->type()) {
                        case storm::storage::DFTElementType::BE_EXP: {
                            STORM_LOG_DEBUG("Transform " + element->name() + " [BE (exp)]");
                            auto be_exp = std::static_pointer_cast<storm::storage::BEExponential<ValueType> const>(
                                    element);
                            builder.addBasicElementExponential(be_exp->name(), be_exp->activeFailureRate(),
                                                               be_exp->dormancyFactor());
                            break;
                        }
                        case storm::storage::DFTElementType::BE_CONST: {
                            auto be_const = std::static_pointer_cast<storm::storage::BEExponential<ValueType> const>(
                                    element);
                            STORM_LOG_DEBUG("Transform " + element->name() + " [BE (const)]");
                            // All original constant BEs are set to failsafe, failed BEs are later triggered by a new element
                            builder.addBasicElementConst(be_const->name(), be_const->canFail());
                            break;
                        }
                        case storm::storage::DFTElementType::AND:
                            STORM_LOG_DEBUG("Transform " + element->name() + " [AND]");
                            builder.addAndElement(element->name(), getChildrenVector(element));
                            break;
                        case storm::storage::DFTElementType::OR:
                            STORM_LOG_DEBUG("Transform " + element->name() + " [OR]");
                            builder.addOrElement(element->name(), getChildrenVector(element));
                            break;
                        case storm::storage::DFTElementType::VOT: {
                            STORM_LOG_DEBUG("Transform " + element->name() + " [VOT]");
                            auto vot = std::static_pointer_cast<storm::storage::DFTVot<ValueType> const>(element);
                            builder.addVotElement(vot->name(), vot->threshold(), getChildrenVector(vot));
                            break;
                        }
                        case storm::storage::DFTElementType::PAND: {
                            STORM_LOG_DEBUG("Transform " + element->name() + " [PAND]");
                            auto pand = std::static_pointer_cast<storm::storage::DFTPand<ValueType> const>(element);
                            builder.addPandElement(pand->name(), getChildrenVector(pand), pand->isInclusive());
                            break;
                        }
                        case storm::storage::DFTElementType::POR: {
                            STORM_LOG_DEBUG("Transform " + element->name() + " [POR]");
                            auto por = std::static_pointer_cast<storm::storage::DFTPor<ValueType> const>(element);
                            builder.addPandElement(por->name(), getChildrenVector(por), por->isInclusive());
                            break;
                        }
                        case storm::storage::DFTElementType::SPARE:
                            STORM_LOG_DEBUG("Transform " + element->name() + " [SPARE]");
                            builder.addSpareElement(element->name(), getChildrenVector(element));
                            break;
                        case storm::storage::DFTElementType::PDEP: {
                            auto dep = std::static_pointer_cast<storm::storage::DFTDependency<ValueType> const>(
                                    element);
                            auto children = getChildrenVector(dep);
                            if (!storm::utility::isOne(dep->probability())) {
                                STORM_LOG_DEBUG("Transform " + element->name() + " [PDEP]");
                                if (children.size() > 2) {
                                    // Introduce additional element for first capturing the probabilistic dependency
                                    std::string nameAdditional = dep->name() + "_additional";
                                    builder.addBasicElementConst(nameAdditional, false);
                                    // First consider probabilistic dependency
                                    builder.addDepElement(dep->name() + "_pdep", {children.front(), nameAdditional},
                                                          dep->probability());
                                    // Then consider dependencies to the children if probabilistic dependency failed
                                    children.erase(children.begin());
                                    size_t i = 1;
                                    for (auto const &child : children) {
                                        std::string nameDep = dep->name() + "_" + std::to_string(i);
                                        if (builder.nameInUse(nameDep)) {
                                            STORM_LOG_ERROR("Element with name '" << nameDep << "' already exists.");
                                        }
                                        builder.addDepElement(nameDep, {dep->name() + "_additional", child},
                                                              storm::utility::one<ValueType>());
                                        ++i;
                                    }
                                } else {
                                    builder.addDepElement(dep->name(), children, dep->probability());
                                }
                            } else {
                                STORM_LOG_DEBUG("Transform " + element->name() + " [FDEP]");
                                // Add dependencies
                                for (size_t i = 1; i < children.size(); ++i) {
                                    std::string nameDep;
                                    if (children.size() == 2) {
                                        nameDep = dep->name();
                                    } else {
                                        nameDep = dep->name() + "_" + std::to_string(i);
                                    }
                                    if (builder.nameInUse(nameDep)) {
                                        STORM_LOG_ERROR("Element with name '" << nameDep << "' already exists.");
                                    }
                                    STORM_LOG_ASSERT(storm::utility::isOne(dep->probability()) || children.size() == 2,
                                                     "PDEP with multiple children supported.");
                                    builder.addDepElement(nameDep, {children[0], children[i]},
                                                          storm::utility::one<ValueType>());
                                }
                            }
                            break;
                        }
                        case storm::storage::DFTElementType::SEQ:
                            STORM_LOG_DEBUG("Transform " + element->name() + " [SEQ]");
                            builder.addSequenceEnforcer(element->name(), getChildrenVector(element));
                            break;
                        case storm::storage::DFTElementType::MUTEX:
                            STORM_LOG_DEBUG("Transform " + element->name() + " [MUTEX]");
                            builder.addMutex(element->name(), getChildrenVector(element));
                            break;
                        default:
                            STORM_LOG_THROW(false, storm::exceptions::NotImplementedException,
                                            "DFT type '" << element->type() << "' not known.");
                            break;
                    }

                }

                builder.setTopLevel(dft.getTopLevelGate()->name());

                STORM_LOG_DEBUG("Transformation complete!");
                return std::make_shared<storm::storage::DFT<ValueType>>(builder.build());
            }

            template<typename ValueType>
            std::vector<std::string> DftTransformator<ValueType>::getChildrenVector(
                    std::shared_ptr<storm::storage::DFTElement<ValueType> const> element) {
                std::vector<std::string> res;
                if (element->isDependency()) {
                    // Dependencies have to be handled separately
                    auto dependency = std::static_pointer_cast<storm::storage::DFTDependency<ValueType> const>(element);
                    res.push_back(dependency->triggerEvent()->name());
                    for (auto const &depEvent : dependency->dependentEvents()) {
                        res.push_back(depEvent->name());
                    }
                } else {
                    auto elementWithChildren = std::static_pointer_cast<storm::storage::DFTChildren<ValueType> const>(
                            element);
                    for (auto const &child : elementWithChildren->children()) {
                        res.push_back(child->name());
                    }
                }
                return res;
            }

            // Explicitly instantiate the class.
            template
            class DftTransformator<double>;

#ifdef STORM_HAVE_CARL

            template
            class DftTransformator<RationalFunction>;

#endif
        }
    }
}
