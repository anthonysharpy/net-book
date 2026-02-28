#pragma once

namespace netbook::concurrency {

void use_unique_core_for_thread();
void pin_thread_to_core(int core_id);

}