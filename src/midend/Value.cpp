//
// Created by noyex on 24-4-7.
//

#include "Value.h"

namespace coy {
    const std::shared_ptr<None> None::INSTANCE = std::make_shared<None>();
    const std::shared_ptr<Integer> Integer::ZERO = std::make_shared<Integer>(0);
    const std::shared_ptr<Integer> Integer::ONE = std::make_shared<Integer>(1);
    const std::shared_ptr<Integer> Integer::MINUS_ONE = std::make_shared<Integer>(-1);
} // coy