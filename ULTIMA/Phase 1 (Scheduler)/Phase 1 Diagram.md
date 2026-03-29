# Phase 1 Diagram

This file turns the former placeholder into source-controlled class diagrams for the Scheduler and Semaphore phase. The diagrams reflect the headers currently shipped in this repository.

## Phase 1 Class Overview

```mermaid
classDiagram
direction LR

class Scheduler
class Semaphore
class U2_ui
class U2_window
class TCB
class TaskSnapshot
class State {
  <<enumeration>>
  RUNNING
  READY
  BLOCKED
  DEAD
}

Scheduler "1" *-- "0..*" TCB : process_table
Scheduler ..> TaskSnapshot : snapshot_tasks()
TCB --> State
TaskSnapshot --> State
Semaphore --> Scheduler : block_current_task()/unblock_task()
U2_ui "1" *-- "0..*" U2_window : window_list
```

## Scheduler Detail

```mermaid
classDiagram
direction TB

class State {
  <<enumeration>>
  RUNNING
  READY
  BLOCKED
  DEAD
}

class TCB {
  +task_id : int
  +task_name : std::string
  +task_state : State
  +task_entry : TaskEntryPoint
  +context_pointer : void*
  +dispatch_count : int
  +yield_count : int
  +block_count : int
  +unblock_count : int
  +detail_note : std::string
  +last_transition : std::string
}

class TaskSnapshot {
  +task_id : int
  +task_name : std::string
  +task_state : State
  +dispatch_count : int
  +yield_count : int
  +block_count : int
  +unblock_count : int
  +detail_note : std::string
  +last_transition : std::string
}

class Scheduler {
  -process_table : vector~TCB*~
  -current_running_task : int
  -next_task_id : int
  -dispatch_in_progress : bool
  -scheduler_tick : int
  -last_scheduler_event : std::string
  -find_task_index(task_id)
  -find_next_ready_index()
  -state_to_string(state)
  +reset()
  +create_task(task_name, func_ptr)
  +kill_task(task_id)
  +yield()
  +garbage_collect()
  +block_task(task_id)
  +unblock_task(task_id)
  +block_current_task()
  +dump(level)
  +snapshot_tasks()
  +describe_run_queue()
}

Scheduler "1" *-- "0..*" TCB : owns and deletes
Scheduler ..> TaskSnapshot : emits read-only state views
TCB --> State
TaskSnapshot --> State
```

## Semaphore Detail

```mermaid
classDiagram
direction TB

class Scheduler {
  +get_current_task_id()
  +get_task_name(task_id)
  +block_current_task()
  +unblock_task(task_id)
}

class Semaphore {
  -resource_name : std::string
  -initial_value : int
  -value : int
  -scheduler : Scheduler*
  -wait_queue : deque~int~
  -owner_task_id : int
  -down_operations : int
  -up_operations : int
  -contention_events : int
  -last_transition : std::string
  +reset()
  +down()
  +up()
  +P()
  +V()
  +dump(level)
  +get_sema_value()
  +get_owner_task_id()
  +waiting_task_count()
  +describe_wait_queue()
}

Semaphore --> Scheduler : scheduler callbacks
```

## UI Detail

```mermaid
classDiagram
direction TB

class WINDOW {
  <<external ncurses type>>
}

class U2_window {
  -win : WINDOW*
  -window_title : std::string
  -h : int
  -w : int
  -start_y : int
  -start_x : int
  -scroll_enabled : bool
  -screen_mutex : pthread_mutex_t (static)
  +render()
  +write_text(text)
  +write_text_at(y, x, text)
  +draw_lines(lines)
  +box_window()
  +clear_window()
  +inner_height()
  +inner_width()
}

class U2_ui {
  -window_list : list~U2_window*~
  +init_ncurses_env()
  +close_ncurses_env()
  +add_window(new_window)
  +delete_window(target_window)
  +refresh_all()
  +clear_all()
}

U2_ui "1" *-- "0..*" U2_window : owns window lifetime
U2_window --> WINDOW : wraps ncurses window
```

## Coverage of Concrete Classes

- `Scheduler`
- `Semaphore`
- `U2_ui`
- `U2_window`

Supporting scheduler state types shown in the diagrams:

- `TCB`
- `TaskSnapshot`
- `State`
