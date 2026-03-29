# ULTIMA 2.0 Combined Design Diagram

This diagram is derived from the current Phase 1 C++ interfaces in the repository. It captures every concrete class in the codebase plus the task data structures that those classes exchange.

```mermaid
classDiagram
direction LR

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
  +reset()
  +create_task(task_name, func_ptr)
  +kill_task(task_id)
  +yield()
  +garbage_collect()
  +block_task(task_id)
  +unblock_task(task_id)
  +dump(level)
  +snapshot_tasks()
  +describe_run_queue()
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
  +describe_wait_queue()
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
  +get_win_ptr()
}

class WINDOW {
  <<external ncurses type>>
}

TCB --> State : stores
TaskSnapshot --> State : stores
Scheduler "1" *-- "0..*" TCB : owns
Scheduler ..> TaskSnapshot : returns snapshots
Semaphore --> Scheduler : blocks and wakes tasks through
U2_ui "1" *-- "0..*" U2_window : manages
U2_window --> WINDOW : wraps
```

## Notes

- `Scheduler`, `Semaphore`, `U2_ui`, and `U2_window` are the four concrete classes currently implemented in the repo.
- `TCB`, `TaskSnapshot`, and `State` are included because they define the scheduler's observable state model and appear in the public interface.
- `WINDOW` is shown as an external dependency from `platform_curses.h`, not as project-owned logic.
