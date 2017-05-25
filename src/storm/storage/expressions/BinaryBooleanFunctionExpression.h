#ifndef STORM_STORAGE_EXPRESSIONS_BINARYBOOLEANFUNCTIONEXPRESSION_H_
#define STORM_STORAGE_EXPRESSIONS_BINARYBOOLEANFUNCTIONEXPRESSION_H_

#include "storm/storage/expressions/BinaryExpression.h"
#include "storm/utility/OsDetection.h"

namespace storm {
    namespace expressions {
        class BinaryBooleanFunctionExpression : public BinaryExpression {
        public:
            /*!
             * An enum type specifying the different operators applicable.
             */
            enum class OperatorType {And, Or, Xor, Implies, Iff};
            
            /*!
             * Creates a binary boolean function expression with the given return type, operands and operator.
             *
             * @param manager The manager responsible for this expression.
             * @param type The return type of the expression.
             * @param firstOperand The first operand of the expression.
             * @param secondOperand The second operand of the expression.
             * @param functionType The operator of the expression.
             */
            BinaryBooleanFunctionExpression(ExpressionManager const& manager, Type const& type, std::shared_ptr<BaseExpression const> const& firstOperand, std::shared_ptr<BaseExpression const> const& secondOperand, OperatorType operatorType);
            
            // Instantiate constructors and assignments with their default implementations.
            BinaryBooleanFunctionExpression(BinaryBooleanFunctionExpression const& other) = default;
            BinaryBooleanFunctionExpression& operator=(BinaryBooleanFunctionExpression const& other) = delete;
            BinaryBooleanFunctionExpression(BinaryBooleanFunctionExpression&&) = default;
            BinaryBooleanFunctionExpression& operator=(BinaryBooleanFunctionExpression&&) = delete;

            virtual ~BinaryBooleanFunctionExpression() = default;
            
            // Override base class methods.
            virtual storm::expressions::OperatorType getOperator() const override;
            virtual bool evaluateAsBool(Valuation const* valuation = nullptr) const override;
            virtual std::shared_ptr<BaseExpression const> simplify() const override;
            virtual boost::any accept(ExpressionVisitor& visitor, boost::any const& data) const override;
            virtual bool isBinaryBooleanFunctionExpression() const override;

            /*!
             * Retrieves the operator associated with the expression.
             *
             * @return The operator associated with the expression.
             */
            OperatorType getOperatorType() const;
            
        protected:
            // Override base class method.
            virtual void printToStream(std::ostream& stream) const override;

        private:
            // The operator of the expression.
            OperatorType operatorType;
        };
    }
}

#endif /* STORM_STORAGE_EXPRESSIONS_BINARYBOOLEANFUNCTIONEXPRESSION_H_ */
