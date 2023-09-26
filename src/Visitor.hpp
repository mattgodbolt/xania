#pragma once

template <typename... Ts>
struct Visitor : Ts... {
    using Ts::operator()...;
};
