#include "src/generator/DftNextStateGenerator.h"

#include "src/utility/constants.h"
#include "src/utility/macros.h"
#include "src/exceptions/NotImplementedException.h"

namespace storm {
    namespace generator {
        
        template<typename ValueType, typename StateType>
        DftNextStateGenerator<ValueType, StateType>::DftNextStateGenerator(storm::storage::DFT<ValueType> const& dft, storm::storage::DFTStateGenerationInfo const& stateGenerationInfo, bool enableDC, bool mergeFailedStates) : mDft(dft), mStateGenerationInfo(stateGenerationInfo), state(nullptr), enableDC(enableDC), mergeFailedStates(mergeFailedStates), comparator() {
            deterministicModel = !mDft.canHaveNondeterminism();
        }
        
        template<typename ValueType, typename StateType>
        bool DftNextStateGenerator<ValueType, StateType>::isDeterministicModel() const {
            return deterministicModel;
        }
        
        template<typename ValueType, typename StateType>
        std::vector<StateType> DftNextStateGenerator<ValueType, StateType>::getInitialStates(StateToIdCallback const& stateToIdCallback) {
            DFTStatePointer initialState = std::make_shared<storm::storage::DFTState<ValueType>>(mDft, mStateGenerationInfo, 0);

            // Register initial state
            StateType id = stateToIdCallback(initialState);

            initialState->setId(id);
            return {id};
        }
        
        template<typename ValueType, typename StateType>
        void DftNextStateGenerator<ValueType, StateType>::load(DFTStatePointer const& state) {
            // TODO Matthias load state from bitvector
            // Store a pointer to the state itself, because we need to be able to access it when expanding it.
            this->state = &state;
        }
        
        template<typename ValueType, typename StateType>
        bool DftNextStateGenerator<ValueType, StateType>::satisfies(storm::expressions::Expression const& expression) const {
            STORM_LOG_THROW(false, storm::exceptions::NotImplementedException, "The method 'satisfies' is not yet implemented.");
        }
        
        template<typename ValueType, typename StateType>
        StateBehavior<ValueType, StateType> DftNextStateGenerator<ValueType, StateType>::expand(StateToIdCallback const& stateToIdCallback) {
            DFTStatePointer currentState = *state;
            STORM_LOG_TRACE("Explore state: " << mDft.getStateString(currentState));

            // Prepare the result, in case we return early.
            StateBehavior<ValueType, StateType> result;

            // Initialization
            bool hasDependencies = currentState->nrFailableDependencies() > 0;
            size_t failableCount = hasDependencies ? currentState->nrFailableDependencies() : currentState->nrFailableBEs();
            size_t currentFailable = 0;
            Choice<ValueType, StateType> choice(0, !hasDependencies);

            // Check for absorbing state
            if (mDft.hasFailed(currentState) || mDft.isFailsafe(currentState) || currentState->nrFailableBEs() == 0) {
                // Add self loop
                choice.addProbability(currentState->getId(), storm::utility::one<ValueType>());
                STORM_LOG_TRACE("Added self loop for " << currentState->getId());
                // No further exploration required
                result.addChoice(std::move(choice));
                result.setExpanded();
                return result;
            }

            // Let BE fail
            while (currentFailable < failableCount) {
                STORM_LOG_ASSERT(!mDft.hasFailed(currentState), "Dft has failed.");

                // Construct new state as copy from original one
                DFTStatePointer newState = std::make_shared<storm::storage::DFTState<ValueType>>(*currentState);
                std::pair<std::shared_ptr<storm::storage::DFTBE<ValueType> const>, bool> nextBEPair = newState->letNextBEFail(currentFailable);
                std::shared_ptr<storm::storage::DFTBE<ValueType> const>& nextBE = nextBEPair.first;
                STORM_LOG_ASSERT(nextBE, "NextBE is null.");
                STORM_LOG_ASSERT(nextBEPair.second == hasDependencies, "Failure due to dependencies does not match.");
                STORM_LOG_TRACE("With the failure of: " << nextBE->name() << " [" << nextBE->id() << "] in " << mDft.getStateString(currentState));

                /*if (storm::settings::getModule<storm::settings::modules::DFTSettings>().computeApproximation()) {
                    if (!storm::utility::isZero(exitRate)) {
                        ValueType rate = nextBE->activeFailureRate();
                        ValueType div = rate / exitRate;
                        if (!storm::utility::isZero(exitRate) && belowThreshold(div)) {
                            // Set transition directly to failed state
                            auto resultFind = outgoingRates.find(failedIndex);
                            if (resultFind != outgoingRates.end()) {
                                // Add to existing transition
                                resultFind->second += rate;
                                STORM_LOG_TRACE("Updated transition to " << resultFind->first << " with rate " << rate << " to new rate " << resultFind->second);
                            } else {
                                // Insert new transition
                                outgoingRates.insert(std::make_pair(failedIndex, rate));
                                STORM_LOG_TRACE("Added transition to " << failedIndex << " with rate " << rate);
                            }
                            exitRate += rate;
                            std::cout << "IGNORE: " << nextBE->name() << " [" << nextBE->id() << "] with rate " << rate << std::endl;
                            //STORM_LOG_TRACE("Ignore: " << nextBE->name() << " [" << nextBE->id() << "] with rate " << rate);
                            continue;
                        }
                    }
                }*/

                // Propagate
                storm::storage::DFTStateSpaceGenerationQueues<ValueType> queues;

                // Propagate failure
                for (DFTGatePointer parent : nextBE->parents()) {
                    if (newState->isOperational(parent->id())) {
                        queues.propagateFailure(parent);
                    }
                }
                // Propagate failures
                while (!queues.failurePropagationDone()) {
                    DFTGatePointer next = queues.nextFailurePropagation();
                    next->checkFails(*newState, queues);
                    newState->updateFailableDependencies(next->id());
                }

                // Check restrictions
                for (DFTRestrictionPointer restr : nextBE->restrictions()) {
                    queues.checkRestrictionLater(restr);
                }
                // Check restrictions
                while(!queues.restrictionChecksDone()) {
                    DFTRestrictionPointer next = queues.nextRestrictionCheck();
                    next->checkFails(*newState, queues);
                    newState->updateFailableDependencies(next->id());
                }

                if(newState->isInvalid()) {
                    // Continue with next possible state
                    ++currentFailable;
                    continue;
                }

                StateType newStateId;

                if (newState->hasFailed(mDft.getTopLevelIndex()) && mergeFailedStates) {
                    // Use unique failed state
                    newStateId = mergeFailedStateId;
                } else {
                    // Propagate failsafe
                    while (!queues.failsafePropagationDone()) {
                        DFTGatePointer next = queues.nextFailsafePropagation();
                        next->checkFailsafe(*newState, queues);
                    }

                    // Propagate dont cares
                    while (enableDC && !queues.dontCarePropagationDone()) {
                        DFTElementPointer next = queues.nextDontCarePropagation();
                        next->checkDontCareAnymore(*newState, queues);
                    }

                    // Update failable dependencies
                    newState->updateFailableDependencies(nextBE->id());
                    newState->updateDontCareDependencies(nextBE->id());

                    // Add new state
                    newStateId = stateToIdCallback(newState);
                }

                // Set transitions
                if (hasDependencies) {
                    // Failure is due to dependency -> add non-deterministic choice
                    std::shared_ptr<storm::storage::DFTDependency<ValueType> const> dependency = mDft.getDependency(currentState->getDependencyId(currentFailable));
                    choice.addProbability(newStateId, dependency->probability());
                    STORM_LOG_TRACE("Added transition to " << newStateId << " with probability " << dependency->probability());

                    if (!storm::utility::isOne(dependency->probability())) {
                        // Add transition to state where dependency was unsuccessful
                        DFTStatePointer unsuccessfulState = std::make_shared<storm::storage::DFTState<ValueType>>(*currentState);
                        unsuccessfulState->letDependencyBeUnsuccessful(currentFailable);
                        // Add state
                        StateType unsuccessfulStateId = stateToIdCallback(unsuccessfulState);
                        ValueType remainingProbability = storm::utility::one<ValueType>() - dependency->probability();
                        choice.addProbability(unsuccessfulStateId, remainingProbability);
                        STORM_LOG_TRACE("Added transition to " << unsuccessfulStateId << " with remaining probability " << remainingProbability);
                    }
                    result.addChoice(std::move(choice));
                } else {
                    // Failure is due to "normal" BE failure
                    // Set failure rate according to activation
                    bool isActive = true;
                    if (mDft.hasRepresentant(nextBE->id())) {
                        // Active must be checked for the state we are coming from as this state is responsible for the
                        // rate and not the new state we are going to
                        isActive = currentState->isActive(mDft.getRepresentant(nextBE->id())->id());
                    }
                    ValueType rate = isActive ? nextBE->activeFailureRate() : nextBE->passiveFailureRate();
                    STORM_LOG_ASSERT(!storm::utility::isZero(rate), "Rate is 0.");
                    choice.addProbability(newStateId, rate);
                    STORM_LOG_TRACE("Added transition to " << newStateId << " with " << (isActive ? "active" : "passive") << " rate " << rate);
                }

                ++currentFailable;
            } // end while failing BE
            
            if (!hasDependencies) {
                // Add all rates as one choice
                result.addChoice(std::move(choice));
            }

            STORM_LOG_TRACE("Finished exploring state: " << mDft.getStateString(currentState));
            result.setExpanded();
            return result;
        }

        template<typename ValueType, typename StateType>
        StateBehavior<ValueType, StateType> DftNextStateGenerator<ValueType, StateType>::createMergeFailedState(StateToIdCallback const& stateToIdCallback) {
            STORM_LOG_ASSERT(mergeFailedStates, "No unique failed state used.");
            // Introduce explicit fail state
            DFTStatePointer failedState = std::make_shared<storm::storage::DFTState<ValueType>>(mDft, mStateGenerationInfo, 0);
            mergeFailedStateId = stateToIdCallback(failedState);
            STORM_LOG_TRACE("Introduce fail state with id: " << mergeFailedStateId);

            // Add self loop
            Choice<ValueType, StateType> choice(0, true);
            choice.addProbability(mergeFailedStateId, storm::utility::one<ValueType>());

            // No further exploration required
            StateBehavior<ValueType, StateType> result;
            result.addChoice(std::move(choice));
            result.setExpanded();
            return result;
        }
        
        template class DftNextStateGenerator<double>;
        template class DftNextStateGenerator<storm::RationalFunction>;
    }
}