#pragma once
#include <benchmark/benchmark.h>
#include <string>

template <typename Impl>
void set_impl_label(benchmark::State& state) {
    state.SetLabel(std::string(Impl::impl_name));
}

template <typename Impl>
void set_impl_label_with_item(benchmark::State& state) {
    std::string label = std::string(Impl::impl_name);
    label += " item=" + std::to_string(sizeof(typename Impl::Self_item_t)) + "B";
    state.SetLabel(label);
}

template <typename Impl>
void set_impl_label_with_key_value(benchmark::State& state) {
    std::string label = std::string(Impl::impl_name);
    label += " key=" + std::to_string(sizeof(typename Impl::Self_key_t)) + "B";
    label += " value=" + std::to_string(sizeof(typename Impl::Self_value_t)) + "B";
    state.SetLabel(label);
}
