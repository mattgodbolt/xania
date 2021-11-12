/************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                     */
/*  (C) 2021 Xania Development Team                                     */
/*  See the header to file: merc.h for original code copyrights         */
/************************************************************************/
#pragma once

#include <optional>
#include <string>

class ObjectIndex;
// An interface for validators that can scrutinise an ObjectIndex after it's
// been loaded from an area file.
class ObjectIndexValidator {
public:
    virtual ~ObjectIndexValidator() = default;

    // The result's error_message is populated if validation failed.
    struct Result {
        std::optional<std::string> error_message;
    };

    [[nodiscard]] virtual Result validate(const ObjectIndex *obj_index) const = 0;

protected:
    const Result Success = ObjectIndexValidator::Result{std::nullopt};
};
