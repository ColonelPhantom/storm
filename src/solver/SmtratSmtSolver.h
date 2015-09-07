#ifndef STORM_SOLVER_SMTRATSMTSOLVER
#define STORM_SOLVER_SMTRATSMTSOLVER
#include "storm-config.h"
#include "src/solver/SmtSolver.h"

#ifdef STORM_HAVE_SMTRAT
#ifdef SMTRATDOESNTWORK // Does not compile with current version of smtrat.

#include "lib/smtrat.h"
#include "../adapters/carlAdapter.h"


namespace storm {
	namespace solver {
		class SmtratSmtSolver : public SmtSolver {
                private:
                        smtrat::RatOne* solver;
                        unsigned exitCode;
                    
		public:
			SmtratSmtSolver(storm::expressions::ExpressionManager& manager);
			virtual ~SmtratSmtSolver();

			virtual void push() override;

			virtual void pop() override;

			virtual void pop(uint_fast64_t n) override;

			virtual CheckResult check() override;

                        void add(storm::RawPolynomial const&, storm::CompareRelation);
                        
                        template<typename ReturnType>
                        ReturnType  getModel() const;
                        
                        std::vector<smtrat::FormulasT> const& getUnsatisfiableCores() const;
                        
                        
                        
                        
                        
            
            // The last result that was returned by any of the check methods.
			CheckResult lastResult;
		};
	}
}
#endif
#endif

#endif // STORM_SOLVER_SMTRATSMTSOLVER