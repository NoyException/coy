//
// Created by noyex on 24-4-8.
//

#include "IRInstruction.h"

namespace coy {
    
    const std::shared_ptr<Expression> Expression::NONE = std::make_shared<Expression>(None::INSTANCE);
    
    const std::shared_ptr<Expression> Expression::ZERO = std::make_shared<Expression>(Integer::ZERO);
    
    const std::shared_ptr<Expression> Expression::ONE = std::make_shared<Expression>(Integer::ONE);
    
    const std::shared_ptr<Expression> Expression::MINUS_ONE = std::make_shared<Expression>(Integer::MINUS_ONE);
} // coy