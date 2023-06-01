#include "coroutine.hh"
#include "schedule.hh"
#include <stdexcept>
#include <cstring>
#include <cassert>

#define MAIN_CTX (Schedule::instance()->main_ctx)
#define MAIN_STACK (Schedule::instance()->main_stack)

namespace coroutine
{
Coroutine::Coroutine() :
  m_buffer(nullptr), m_stack_size(0),
  m_capacity(0), m_status(co_status::READY), m_id(-1) {
}

Coroutine::~Coroutine() {
  if (m_buffer) delete[] m_buffer;
}

void Coroutine::resume() {
  switch (m_status) {
  case co_status::READY :
    m_status = co_status::RUNNING;
    swapcontext(&MAIN_CTX, &m_ctx);
    break;
  case co_status::SUSPEND :
    m_status = co_status::RUNNING;
    load_stack();
    swapcontext(&MAIN_CTX, &m_ctx);
    break;
  default:
    // RUNNING and DEAD is here
    throw std::runtime_error("RUNNING and DEAD coroutine cannot reach here");
  }
}

void Coroutine::yield() {
  if (m_status != co_status::RUNNING) return;

  m_status = co_status::SUSPEND;
  save_stack();
  swapcontext(&m_ctx, &MAIN_CTX);
}

void Coroutine::destroy() {
  yield();
  m_status = co_status::DEAD;
}

void Coroutine::save_stack() {
  const char *stack_bottom = Schedule::instance()->get_stack_bottom();
  char dummy = 0;

  auto used_stack = stack_bottom - &dummy;
  assert(used_stack <= 1024 * 1024);

  if (used_stack > m_capacity) {
    if (m_buffer) delete[] m_buffer;
    m_capacity = used_stack;
    m_buffer = new char[m_capacity];
  }

  m_stack_size = used_stack;
  memcpy(m_buffer, &dummy, m_stack_size);
}

void Coroutine::load_stack() {
  void *p = Schedule::instance()->get_stack_bottom() - m_stack_size;
  memcpy(p, m_buffer, m_stack_size);
}

}  // namespace coroutine