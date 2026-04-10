/* Team Thunder reference artwork */
/* Creator: ZANDER HAYES - TEAM THUNDER */
/* Phase Label: Phase 1 - Scheduler and Semaphore */

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <csignal>
#include <thread>
#include <chrono>
#include <vector>

#if defined(_WIN32)
#include <direct.h>
#include <io.h>
#include <process.h>
#else
#include <unistd.h>
#endif
#include "Mem_mgr.h"
#include "platform_curses.h"
#include "U2_Scheduler.h"
#include "Sema.h"
#include "U2_UI.h"
#include "U2_Window.h"

/**
 * ULTIMA 2.0 - Phase 1 Main Driver
 * The presentation layer intentionally keeps the scheduler, semaphore, state,
 * race-condition proof, and per-task traces visible at the same time so the
 * Phase 1 requirements are easy to explain in screenshots and a live demo.
 */

Scheduler sys_scheduler;
Semaphore printer_semaphore("Printer_Output", 1, &sys_scheduler);
Mem_mgr proof_memory_manager(1024);
U2_ui ui_manager;

U2_window* header_window = nullptr;
U2_window* scheduler_window = nullptr;
U2_window* semaphore_window = nullptr;
U2_window* state_window = nullptr;
U2_window* task_window_a = nullptr;
U2_window* task_window_b = nullptr;
U2_window* task_window_c = nullptr;
U2_window* log_window = nullptr;
U2_window* console_window = nullptr;

namespace {

constexpr int kMinimumTerminalRows = 24;
constexpr int kMinimumTerminalCols = 80;
constexpr int kPreferredPresentationRows = 62;
constexpr int kPreferredPresentationCols = 140;
constexpr int kHorizontalMargin = 1;
constexpr int kHorizontalGap = 1;
constexpr int kCompactHeaderHeight = 4;
constexpr int kReferenceHeaderHeight = 5;
constexpr int kReferencePrimaryPanelHeight = 7;
constexpr int kReferenceBottomHeight = 6;
constexpr int kReferenceLogPanelHeight = 9;
constexpr int kReferenceConsoleMinWidth = 28;
constexpr int kReferenceConsoleMaxWidth = 34;
constexpr int kMinimumCompactColumnWidth = 24;
constexpr int kStepPauseMs = 900;
constexpr int kDispatchPauseMs = 250;
constexpr int kDumpPauseMs = 1500;
constexpr int kCycleRestartPauseMs = 2200;
constexpr std::size_t kMaxHistoryLines = 18;
constexpr std::size_t kMaxStateTraceLines = 12;
constexpr std::size_t kTaskABufferBytes = 160;
constexpr std::size_t kTaskBBufferBytes = 112;
constexpr std::size_t kTaskCBufferBytes = 96;

struct WindowLayout {
    bool stacked_primary_layout = false;
    bool full_width_panels = false;

    int header_height = 0;
    int header_width = 0;
    int header_y = 0;
    int header_x = 0;

    int class_height = 0;
    int class_y = 0;
    int class_width_left = 0;
    int class_width_middle = 0;
    int class_width_right = 0;
    int class_x_left = 0;
    int class_x_middle = 0;
    int class_x_right = 0;

    int task_height = 0;
    int task_y = 0;
    int task_width_left = 0;
    int task_width_middle = 0;
    int task_width_right = 0;
    int task_x_left = 0;
    int task_x_middle = 0;
    int task_x_right = 0;

    int bottom_height = 0;
    int bottom_y = 0;
    int log_width = 0;
    int log_x = 0;
    int console_width = 0;
    int console_x = 0;

    int semaphore_y = 0;
    int state_y = 0;
    int state_height = 0;
    int task_a_y = 0;
    int task_b_y = 0;
    int task_c_y = 0;
    int log_y = 0;
    int log_height = 0;
    int console_y = 0;
    int console_height = 0;
};

WindowLayout current_layout;

// Phase 1 is a curses-driven demo by default; transcript mode is opt-in.
bool transcript_only_mode = false;
bool stop_after_cycle = false;
bool demo_paused = false;
bool auto_print_transcript_after_ui = true;
bool continuous_demo_requested = false;

int demo_cycle_number = 1;

int task_a_id = -1;
int task_b_id = -1;
int task_c_id = -1;

int task_a_phase = 0;
int task_b_phase = 0;
int task_c_phase = 0;

bool task_b_was_blocked = false;
bool task_c_was_blocked = false;
bool mid_execution_dump_written = false;
bool post_release_dump_written = false;

std::string console_status = "Phase 1 demo ready.";
std::string latest_flow_summary = "No scheduler activity has executed yet.";
std::string latest_memory_event = "Heap ready; no task buffers allocated.";

std::size_t task_a_buffer_address = Mem_mgr::kInvalidAddress;
std::size_t task_b_buffer_address = Mem_mgr::kInvalidAddress;
std::size_t task_c_buffer_address = Mem_mgr::kInvalidAddress;

std::vector<std::string> transcript_lines;
std::vector<std::string> state_trace_lines;
std::vector<std::string> task_a_history;
std::vector<std::string> task_b_history;
std::vector<std::string> task_c_history;

bool build_layout(WindowLayout& layout) {
    layout = {};

    if (LINES < kMinimumTerminalRows || COLS < kMinimumTerminalCols) {
        return false;
    }

    const int horizontal_margin = (COLS >= 100) ? kHorizontalMargin : 0;
    const int horizontal_gap = (COLS >= 100) ? kHorizontalGap : 0;
    const int full_width = COLS - (horizontal_margin * 2);

    if (LINES >= kPreferredPresentationRows && COLS >= kPreferredPresentationCols) {
        layout.full_width_panels = true;

        layout.header_y = 0;
        layout.header_x = kHorizontalMargin;
        layout.header_height = 5;
        layout.header_width = COLS - (kHorizontalMargin * 2);

        layout.class_y = 5;
        layout.class_height = 7;
        layout.class_width_left = layout.header_width;
        layout.class_width_middle = layout.header_width;
        layout.class_width_right = layout.header_width;
        layout.class_x_left = kHorizontalMargin;
        layout.class_x_middle = kHorizontalMargin;
        layout.class_x_right = kHorizontalMargin;

        layout.semaphore_y = 12;
        layout.state_y = 19;
        layout.state_height = 7;

        layout.task_y = 26;
        layout.task_height = 7;
        layout.task_width_left = layout.header_width;
        layout.task_width_middle = layout.header_width;
        layout.task_width_right = layout.header_width;
        layout.task_x_left = kHorizontalMargin;
        layout.task_x_middle = kHorizontalMargin;
        layout.task_x_right = kHorizontalMargin;
        layout.task_a_y = 26;
        layout.task_b_y = 33;
        layout.task_c_y = 40;

        layout.log_y = 47;
        layout.log_height = 9;
        layout.log_width = layout.header_width;
        layout.log_x = kHorizontalMargin;

        layout.console_y = 56;
        layout.console_height = 6;
        layout.console_width = layout.header_width;
        layout.console_x = kHorizontalMargin;

        layout.bottom_y = layout.log_y;
        layout.bottom_height = layout.log_height + layout.console_height;

        return true;
    }

    constexpr int full_width_required_rows =
        kReferenceHeaderHeight
        + (kReferencePrimaryPanelHeight * 6)
        + kReferenceLogPanelHeight
        + kReferenceBottomHeight;

    if (LINES >= full_width_required_rows) {
        layout.full_width_panels = true;

        layout.header_y = 0;
        layout.header_x = horizontal_margin;
        layout.header_height = kReferenceHeaderHeight;
        layout.header_width = full_width;

        layout.class_y = layout.header_height;
        layout.class_height = kReferencePrimaryPanelHeight;
        layout.class_width_left = full_width;
        layout.class_width_middle = full_width;
        layout.class_width_right = full_width;
        layout.class_x_left = horizontal_margin;
        layout.class_x_middle = horizontal_margin;
        layout.class_x_right = horizontal_margin;

        layout.semaphore_y = layout.class_y + layout.class_height;
        layout.state_y = layout.semaphore_y + layout.class_height;
        layout.state_height = kReferencePrimaryPanelHeight;

        layout.task_height = kReferencePrimaryPanelHeight;
        layout.task_width_left = full_width;
        layout.task_width_middle = full_width;
        layout.task_width_right = full_width;
        layout.task_x_left = horizontal_margin;
        layout.task_x_middle = horizontal_margin;
        layout.task_x_right = horizontal_margin;

        layout.task_a_y = layout.state_y + layout.state_height;
        layout.task_b_y = layout.task_a_y + layout.task_height;
        layout.task_c_y = layout.task_b_y + layout.task_height;
        layout.task_y = layout.task_a_y;

        layout.log_y = layout.task_c_y + layout.task_height;
        layout.log_height = kReferenceLogPanelHeight + (LINES - full_width_required_rows);
        layout.log_width = full_width;
        layout.log_x = horizontal_margin;

        layout.console_y = layout.log_y + layout.log_height;
        layout.console_height = kReferenceBottomHeight;
        layout.console_width = full_width;
        layout.console_x = horizontal_margin;

        layout.bottom_y = layout.log_y;
        layout.bottom_height = layout.console_height;

        return true;
    }

    const int usable_width = COLS - (horizontal_margin * 2) - (horizontal_gap * 2);
    const int column_width = usable_width / 3;
    const int column_remainder = usable_width - (column_width * 3);

    if (column_width < kMinimumCompactColumnWidth) {
        return false;
    }

    layout.header_y = 0;
    layout.header_x = horizontal_margin;
    layout.header_height = (LINES <= kMinimumTerminalRows) ? kCompactHeaderHeight : kReferenceHeaderHeight;
    layout.header_width = COLS - (horizontal_margin * 2);

    layout.class_y = layout.header_height;
    layout.class_height = kReferencePrimaryPanelHeight;
    layout.task_y = layout.class_y + layout.class_height;
    layout.task_height = kReferencePrimaryPanelHeight;
    layout.bottom_y = layout.task_y + layout.task_height;
    layout.bottom_height = kReferenceBottomHeight;

    if (layout.bottom_y + layout.bottom_height > LINES) {
        return false;
    }

    layout.class_width_left = column_width;
    layout.class_width_middle = column_width;
    layout.class_width_right = column_width + column_remainder;
    layout.class_x_left = horizontal_margin;
    layout.class_x_middle = layout.class_x_left + layout.class_width_left + horizontal_gap;
    layout.class_x_right = layout.class_x_middle + layout.class_width_middle + horizontal_gap;

    layout.task_width_left = layout.class_width_left;
    layout.task_width_middle = layout.class_width_middle;
    layout.task_width_right = layout.class_width_right;
    layout.task_x_left = layout.class_x_left;
    layout.task_x_middle = layout.class_x_middle;
    layout.task_x_right = layout.class_x_right;

    const int bottom_width = COLS - (horizontal_margin * 2) - horizontal_gap;
    layout.console_width = std::max(
        kReferenceConsoleMinWidth,
        std::min(bottom_width / 4, kReferenceConsoleMaxWidth)
    );
    layout.log_width = bottom_width - layout.console_width;
    layout.log_x = horizontal_margin;
    layout.console_x = layout.log_x + layout.log_width + horizontal_gap;

    layout.semaphore_y = layout.class_y;
    layout.state_y = layout.class_y;
    layout.state_height = layout.class_height;
    layout.task_a_y = layout.task_y;
    layout.task_b_y = layout.task_y;
    layout.task_c_y = layout.task_y;
    layout.log_y = layout.bottom_y;
    layout.log_height = layout.bottom_height;
    layout.console_y = layout.bottom_y;
    layout.console_height = layout.bottom_height;

    return layout.log_width >= 40;
}

#if !defined(ULTIMA_ENABLE_CURSES) || defined(__APPLE__)
bool file_exists(const std::string& path);
#endif

std::string join_path(const std::string& directory, const std::string& filename) {
    if (directory.empty()) {
        return filename;
    }
    const char last_char = directory[directory.size() - 1];
    if (last_char == '/' || last_char == '\\') {
        return directory + filename;
    }
#if defined(_WIN32)
    return directory + "\\" + filename;
#else
    return directory + "/" + filename;
#endif
}

std::string current_working_directory() {
    char buffer[4096];
#if defined(_WIN32)
    if (_getcwd(buffer, static_cast<int>(sizeof(buffer))) == nullptr) {
        return "";
    }
#else
    if (getcwd(buffer, sizeof(buffer)) == nullptr) {
        return "";
    }
#endif
    return {buffer};
}

#if !defined(ULTIMA_ENABLE_CURSES)
std::string shell_quote_single(const std::string& text) {
    std::string quoted = "'";
    for (char ch : text) {
        if (ch == '\'') {
            quoted += "'\\''";
        } else {
            quoted.push_back(ch);
        }
    }
    quoted.push_back('\'');
    return quoted;
}

bool read_file_prefix(const std::string& path, unsigned char* buffer, std::size_t size) {
    std::ifstream input(path.c_str(), std::ios::binary);
    if (!input.is_open()) {
        return false;
    }

    input.read(reinterpret_cast<char*>(buffer), static_cast<std::streamsize>(size));
    return static_cast<std::size_t>(input.gcount()) >= size;
}

bool binary_looks_runnable_on_this_platform(const std::string& path) {
    unsigned char prefix[4] = {0, 0, 0, 0};
    if (!read_file_prefix(path, prefix, sizeof(prefix))) {
        return false;
    }

#if defined(__APPLE__)
    const bool is_macho =
        (prefix[0] == 0xfe && prefix[1] == 0xed && prefix[2] == 0xfa && prefix[3] == 0xce) ||
        (prefix[0] == 0xce && prefix[1] == 0xfa && prefix[2] == 0xed && prefix[3] == 0xfe) ||
        (prefix[0] == 0xfe && prefix[1] == 0xed && prefix[2] == 0xfa && prefix[3] == 0xcf) ||
        (prefix[0] == 0xcf && prefix[1] == 0xfa && prefix[2] == 0xed && prefix[3] == 0xfe) ||
        (prefix[0] == 0xca && prefix[1] == 0xfe && prefix[2] == 0xba && prefix[3] == 0xbe) ||
        (prefix[0] == 0xbe && prefix[1] == 0xba && prefix[2] == 0xfe && prefix[3] == 0xca);
    return is_macho;
#elif defined(_WIN32)
    return prefix[0] == 'M' && prefix[1] == 'Z';
#else
    const bool is_elf =
        prefix[0] == 0x7f && prefix[1] == 'E' && prefix[2] == 'L' && prefix[3] == 'F';
    return is_elf;
#endif
}

bool try_rebuild_native_curses_binary(const std::string& binary_directory) {
#if defined(_WIN32) && !defined(__CYGWIN__)
    (void) binary_directory;
    return false;
#else
    const std::string makefile_path = join_path(binary_directory, "Makefile");
    if (!file_exists(makefile_path)) {
        return false;
    }

    const std::string rebuild_command =
        "make -s -C " + shell_quote_single(binary_directory) + " build >/dev/null 2>&1";
    return std::system(rebuild_command.c_str()) == 0;
#endif
}

#endif

#if !defined(ULTIMA_ENABLE_CURSES) || defined(__APPLE__)
bool file_exists(const std::string& path) {
#if defined(_WIN32)
    return _access(path.c_str(), 0) == 0;
#else
    return access(path.c_str(), F_OK) == 0;
#endif
}

std::string resolve_invoked_binary_directory(char* argv[]) {
    if (argv == nullptr || argv[0] == nullptr) {
        return current_working_directory();
    }

    std::string invoked_path = argv[0];
    if (invoked_path.empty()) {
        return current_working_directory();
    }

    std::string resolved_path = invoked_path;
    if (invoked_path.find('/') == std::string::npos && invoked_path.find('\\') == std::string::npos) {
        const std::string cwd = current_working_directory();
        if (!cwd.empty()) {
            resolved_path = join_path(cwd, invoked_path);
        }
    }

    const std::size_t separator_index = resolved_path.find_last_of("/\\");
    return (separator_index == std::string::npos)
        ? current_working_directory()
        : resolved_path.substr(0, separator_index);
}
#endif

#if defined(ULTIMA_ENABLE_CURSES)
std::string to_lower_copy(std::string text) {
    for (char& character : text) {
        character = static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
    }
    return text;
}

#if defined(__APPLE__)
std::string trim_copy(const std::string& text) {
    std::size_t begin = 0;
    while (begin < text.size() && std::isspace(static_cast<unsigned char>(text[begin])) != 0) {
        ++begin;
    }

    std::size_t end = text.size();
    while (end > begin && std::isspace(static_cast<unsigned char>(text[end - 1])) != 0) {
        --end;
    }

    return text.substr(begin, end - begin);
}

FILE* open_command_pipe(const std::string& command) {
#if defined(_WIN32) && !defined(__CYGWIN__)
    return _popen(command.c_str(), "r");
#else
    return ::popen(command.c_str(), "r");
#endif
}

int close_command_pipe(FILE* pipe) {
#if defined(_WIN32) && !defined(__CYGWIN__)
    return _pclose(pipe);
#else
    return ::pclose(pipe);
#endif
}

std::string read_command_output(const std::string& command) {
    FILE* pipe = open_command_pipe(command);
    if (pipe == nullptr) {
        return "";
    }

    std::string output;
    char buffer[256];
    while (std::fgets(buffer, static_cast<int>(sizeof(buffer)), pipe) != nullptr) {
        output += buffer;
    }
    close_command_pipe(pipe);
    return trim_copy(output);
}

long process_parent_pid(long process_id) {
    std::ostringstream command;
    command << "/bin/ps -p " << process_id << " -o ppid=";
    return std::strtol(read_command_output(command.str()).c_str(), nullptr, 10);
}

std::string process_command_line(long process_id) {
    std::ostringstream command;
    command << "/bin/ps -p " << process_id << " -o command=";
    return to_lower_copy(read_command_output(command.str()));
}
#endif

#if defined(__APPLE__)
//noinspection SpellCheckingInspection
bool terminal_program_is_known_good() {
    const char* term_program = std::getenv("TERM_PROGRAM");
    if (term_program == nullptr || term_program[0] == '\0') {
        return false;
    }

    const std::string program = to_lower_copy(term_program);
    return program == "apple_terminal"
        || program == "iterm.app"
        || program == "wezterm"
        || program == "vscode";
}
#endif

bool terminal_supports_resize_request() {
    if (std::getenv("WT_SESSION") != nullptr || std::getenv("CYGWIN") != nullptr) {
        return true;
    }

    const char* term_value = std::getenv("TERM");
    if (term_value != nullptr && term_value[0] != '\0') {
        const std::string term_name = to_lower_copy(term_value);
        if (term_name.find("xterm") != std::string::npos
            || term_name.find("screen") != std::string::npos
            || term_name.find("tmux") != std::string::npos
            || term_name.find("rxvt") != std::string::npos
            || term_name.find("cygwin") != std::string::npos
            || term_name.find("mintty") != std::string::npos
            || term_name.find("wezterm") != std::string::npos) {
            return true;
        }
    }

    const char* term_program = std::getenv("TERM_PROGRAM");
    if (term_program == nullptr || term_program[0] == '\0') {
        return false;
    }

    const std::string program = to_lower_copy(term_program);
    return program == "apple_terminal"
        || program == "iterm.app"
        || program == "wezterm";
}

#if defined(__APPLE__)
bool running_inside_jetbrains_debugger() {
    long process_id = static_cast<long>(getppid());
    for (int depth = 0; depth < 3 && process_id > 1; ++depth) {
        const std::string command = process_command_line(process_id);
        if (command.find("clion") != std::string::npos
            || command.find("jetbrains") != std::string::npos
            || command.find("lldb") != std::string::npos
            || command.find("debugserver") != std::string::npos) {
            return true;
        }
        process_id = process_parent_pid(process_id);
    }
    return false;
}
#endif

bool environment_supports_curses_ui() {
    const char* term_value = std::getenv("TERM");
    if (term_value == nullptr || term_value[0] == '\0') {
        return false;
    }

    const std::string term_name(term_value);
    if (term_name == "dumb" || term_name == "unknown") {
        return false;
    }

#if defined(_WIN32)
    return _isatty(_fileno(stdin)) != 0 && _isatty(_fileno(stdout)) != 0;
#else
    return isatty(STDIN_FILENO) != 0 && isatty(STDOUT_FILENO) != 0;
#endif
}

void request_preferred_terminal_size_if_needed() {
    if (continuous_demo_requested) {
        return;
    }

    if (!terminal_supports_resize_request()) {
        return;
    }

#if defined(_WIN32)
    if (_isatty(_fileno(stdout)) == 0) {
        return;
    }
#else
    if (isatty(STDOUT_FILENO) == 0) {
        return;
    }
#endif

    std::cout << "\033[8;" << kPreferredPresentationRows << ';' << kPreferredPresentationCols << 't' << std::flush;
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
}

#if defined(__APPLE__)
bool try_open_external_terminal_launcher(char* argv[]) {
    if (argv == nullptr || argv[0] == nullptr) {
        return false;
    }
    if (std::getenv("ULTIMA_OPENED_EXTERNAL_TERMINAL") != nullptr) {
        return false;
    }

    const std::string binary_directory = resolve_invoked_binary_directory(argv);
    const std::string launcher_path = join_path(binary_directory, "run_ultima_ui.command");

    if (!file_exists(launcher_path)) {
        return false;
    }

    pid_t child_process = fork();
    if (child_process < 0) {
        return false;
    }

    if (child_process == 0) {
        execl("/usr/bin/open", "open", "-a", "Terminal", launcher_path.c_str(), static_cast<char*>(nullptr));
        _exit(127);
    }
    return true;
}
#endif
#endif

#if !defined(ULTIMA_ENABLE_CURSES)
bool try_launch_sibling_curses_binary(int argc, char* argv[]) {
    if (argv == nullptr || argv[0] == nullptr) {
        return false;
    }

    const std::string binary_directory = resolve_invoked_binary_directory(argv);
#if defined(_WIN32)
    const std::string sibling_binary = join_path(binary_directory, "ultima_os.exe");
#else
    const std::string sibling_binary = join_path(binary_directory, "ultima_os");
#endif

    if (!file_exists(sibling_binary)) {
        return false;
    }

    if (!binary_looks_runnable_on_this_platform(sibling_binary)) {
        return false;
    }

    std::vector<std::string> forwarded_arguments;
    forwarded_arguments.reserve(static_cast<std::size_t>(argc));
    forwarded_arguments.push_back(sibling_binary);
    for (int index = 1; index < argc; ++index) {
        forwarded_arguments.emplace_back(argv[index] != nullptr ? argv[index] : "");
    }

    std::vector<char*> raw_arguments;
    raw_arguments.reserve(forwarded_arguments.size() + 1);
    for (std::string& argument : forwarded_arguments) {
        raw_arguments.push_back(const_cast<char*>(argument.c_str()));
    }
    raw_arguments.push_back(nullptr);

#if defined(_WIN32)
    _execv(raw_arguments.front(), raw_arguments.data());
#else
    execv(raw_arguments.front(), raw_arguments.data());
#endif
    return false;
}
#endif

std::string state_to_text(State state) {
    switch (state) {
        case RUNNING:
            return "RUNNING";
        case READY:
            return "READY";
        case BLOCKED:
            return "BLOCKED";
        case DEAD:
            return "DEAD";
        default:
            return "UNKNOWN";
    }
}

void push_bounded_line(std::vector<std::string>& lines, const std::string& line, std::size_t max_lines) {
    lines.push_back(line);
    if (lines.size() > max_lines) {
        lines.erase(lines.begin(), lines.begin() + static_cast<std::ptrdiff_t>(lines.size() - max_lines));
    }
}

void append_transcript(const std::string& line) {
    transcript_lines.push_back(line);
}

void add_state_trace(const std::string& line) {
    push_bounded_line(state_trace_lines, line, kMaxStateTraceLines);
}

void add_task_history(std::vector<std::string>& history, const std::string& line) {
    push_bounded_line(history, line, kMaxHistoryLines);
}

std::string build_transcript_text() {
    std::ostringstream transcript_stream;
    transcript_stream << "ULTIMA 2.0 - Phase 1 transcript\n";
    transcript_stream << "================================\n";

    for (const std::string& line : transcript_lines) {
        transcript_stream << line << '\n';
    }

    return transcript_stream.str();
}

std::string capture_dump_text(const std::function<void()>& dump_function) {
    std::ostringstream capture_stream;
    std::streambuf* original_buffer = std::cout.rdbuf(capture_stream.rdbuf());
    dump_function();
    std::cout.rdbuf(original_buffer);
    return capture_stream.str();
}

const TaskSnapshot* find_snapshot(const std::vector<TaskSnapshot>& snapshots, int task_id) {
    const auto match = std::find_if(snapshots.begin(), snapshots.end(), [task_id](const TaskSnapshot& snapshot) {
        return snapshot.task_id == task_id;
    });

    return (match != snapshots.end()) ? &(*match) : nullptr;
}

std::string format_task_id(int task_id) {
    return (task_id >= 0) ? "T-" + std::to_string(task_id) : "[not created]";
}

bool use_compact_panels() {
    if (current_layout.full_width_panels) {
        return false;
    }

    if (current_layout.stacked_primary_layout) {
        return false;
    }

    return current_layout.class_width_left <= 28 || current_layout.console_width <= 22;
}

std::size_t panel_text_limit(int panel_width) {
    return static_cast<std::size_t>(std::max(8, panel_width - 4));
}

std::string abbreviate_text(const std::string& text, std::size_t limit) {
    if (text.size() <= limit) {
        return text;
    }

    if (limit <= 3) {
        return text.substr(0, limit);
    }

    return text.substr(0, limit - 3) + "...";
}

std::string fit_panel_text(const std::string& text, int panel_width) {
    return abbreviate_text(text, panel_text_limit(panel_width));
}

std::string fit_prefixed_panel_text(const std::string& prefix, const std::string& text, int panel_width) {
    const std::size_t limit = panel_text_limit(panel_width);
    if (prefix.size() >= limit) {
        return abbreviate_text(prefix, limit);
    }

    return prefix + abbreviate_text(text, limit - prefix.size());
}

std::string compact_state_text(State state) {
    switch (state) {
        case RUNNING:
            return "RUN";
        case READY:
            return "RDY";
        case BLOCKED:
            return "BLK";
        case DEAD:
            return "DED";
        default:
            return "UNK";
    }
}

std::string proof_board_queue_summary() {
    const std::vector<TaskSnapshot> snapshots = sys_scheduler.snapshot_tasks();
    std::ostringstream queue_stream;
    bool wrote_any_task = false;

    for (const TaskSnapshot& snapshot : snapshots) {
        if (snapshot.task_state == DEAD) {
            continue;
        }

        if (wrote_any_task) {
            queue_stream << " -> ";
        }

        queue_stream << snapshot.task_name << " " << compact_state_text(snapshot.task_state);
        wrote_any_task = true;
    }

    return wrote_any_task ? queue_stream.str() : "[empty]";
}

std::string compact_scheduler_queue() {
    const std::vector<TaskSnapshot> snapshots = sys_scheduler.snapshot_tasks();
    std::ostringstream queue_stream;
    bool wrote_any_task = false;

    for (const TaskSnapshot& snapshot : snapshots) {
        if (snapshot.task_state == DEAD) {
            continue;
        }

        if (wrote_any_task) {
            queue_stream << ">";
        }

        queue_stream << format_task_id(snapshot.task_id) << ":" << compact_state_text(snapshot.task_state);
        wrote_any_task = true;
    }

    return wrote_any_task ? queue_stream.str() : "[empty]";
}

std::string compact_scheduler_event() {
    std::string event = sys_scheduler.get_last_scheduler_event();
    const int task_id = sys_scheduler.get_current_task_id();

    if (event.find("Dispatching") != std::string::npos) {
        return "dispatch " + format_task_id(task_id);
    }
    if (event.find("Blocked") != std::string::npos) {
        return "blocked task";
    }
    if (event.find("Unblocked") != std::string::npos) {
        return "woke waiter";
    }
    if (event.find("Marked") != std::string::npos) {
        return "marked dead";
    }
    if (event.find("Created") != std::string::npos) {
        return "created task";
    }
    if (event.find("Garbage collection") != std::string::npos) {
        return "garbage collect";
    }

    return event;
}

std::string compact_semaphore_transition() {
    std::string transition = printer_semaphore.get_last_transition();
    if (transition.find("queued") != std::string::npos) {
        return "queued waiter";
    }
    if (transition.find("woke") != std::string::npos) {
        return "woke waiter";
    }
    if (transition.find("available") != std::string::npos) {
        return "resource free";
    }
    if (transition.find("acquired") != std::string::npos) {
        return "resource owned";
    }

    return transition;
}

std::string format_memory_address(std::size_t address) {
    return (address == Mem_mgr::kInvalidAddress) ? "--" : ("@" + std::to_string(address));
}

std::string memory_usage_summary() {
    std::ostringstream memory_stream;
    memory_stream << "used=" << proof_memory_manager.used_bytes()
                  << "/" << proof_memory_manager.capacity()
                  << " free=" << proof_memory_manager.free_bytes()
                  << " alloc=" << proof_memory_manager.allocation_count();
    return memory_stream.str();
}

std::string compact_memory_usage() {
    std::ostringstream memory_stream;
    memory_stream << "M " << proof_memory_manager.used_bytes()
                  << "/" << proof_memory_manager.capacity()
                  << " A" << proof_memory_manager.allocation_count();
    return memory_stream.str();
}

std::string memory_buffer_summary() {
    std::ostringstream buffer_stream;
    buffer_stream << "A=" << format_memory_address(task_a_buffer_address)
                  << " B=" << format_memory_address(task_b_buffer_address)
                  << " C=" << format_memory_address(task_c_buffer_address);
    return buffer_stream.str();
}

std::string latest_history_line(const std::vector<std::string>& history) {
    return history.empty() ? "Awaiting first event." : history.back();
}

std::string format_owner_label() {
    const int owner_task_id = printer_semaphore.get_owner_task_id();
    if (owner_task_id < 0) {
        return "[None]";
    }

    std::ostringstream owner_stream;
    owner_stream << sys_scheduler.get_task_name(owner_task_id)
                 << " (" << format_task_id(owner_task_id) << ")";
    return owner_stream.str();
}

void write_block(U2_window* target_window, const std::string& block) {
    if (target_window != nullptr) {
        target_window->write_text(block.c_str());
        ui_manager.refresh_all();
    }
}

void write_line(U2_window* target_window, const std::string& line) {
    write_block(target_window, line + "\n");
}

void render_header() {
    if (header_window == nullptr) {
        return;
    }

    if (use_compact_panels()) {
        header_window->draw_lines({
            "Phase 1: scheduler + semaphore + memory",
            "Cycle " + std::to_string(demo_cycle_number) + " | " + latest_flow_summary
        });
        return;
    }

    header_window->draw_lines({
        fit_panel_text("ULTIMA 2.0 / Phase 1", current_layout.header_width),
        fit_panel_text("ULTIMA 2.0 / Phase 1 - Scheduler, Semaphore, and Memory", current_layout.header_width),
        fit_panel_text("Purpose: the scheduler scans READY tasks, dispatches one RUNNING task, and keeps BLOCKED tasks off the CPU.", current_layout.header_width)
    });
}

void render_scheduler_panel() {
    if (scheduler_window == nullptr) {
        return;
    }

    if (use_compact_panels()) {
        scheduler_window->draw_lines({
            "Round robin READY scan",
            "Now " + format_task_id(sys_scheduler.get_current_task_id()) + " | tk " + std::to_string(sys_scheduler.get_scheduler_tick()),
            "R" + std::to_string(sys_scheduler.get_ready_task_count())
                + " B" + std::to_string(sys_scheduler.get_blocked_task_count())
                + " D" + std::to_string(sys_scheduler.get_dead_task_count())
                + " A" + std::to_string(sys_scheduler.get_active_task_count()),
            "Q " + abbreviate_text(compact_scheduler_queue(), panel_text_limit(current_layout.class_width_left)),
            "Last " + abbreviate_text(compact_scheduler_event(), panel_text_limit(current_layout.class_width_left))
        });
        return;
    }

    scheduler_window->draw_lines({
        fit_panel_text("Purpose: select the next READY task", current_layout.class_width_left),
        fit_panel_text("with strict round robin.", current_layout.class_width_left),
        fit_prefixed_panel_text(
            "Current: ",
            format_task_id(sys_scheduler.get_current_task_id()) + " | Tick: " + std::to_string(sys_scheduler.get_scheduler_tick()),
            current_layout.class_width_left
        ),
        fit_prefixed_panel_text(
            "Counts: ",
            "READY=" + std::to_string(sys_scheduler.get_ready_task_count())
                + " BLOCKED=" + std::to_string(sys_scheduler.get_blocked_task_count())
                + " DEAD=" + std::to_string(sys_scheduler.get_dead_task_count())
                + " ACTIVE=" + std::to_string(sys_scheduler.get_active_task_count()),
            current_layout.class_width_left
        ),
        fit_prefixed_panel_text("Queue: ", proof_board_queue_summary(), current_layout.class_width_left)
    });
}

void render_semaphore_panel() {
    if (semaphore_window == nullptr) {
        return;
    }

    if (use_compact_panels()) {
        semaphore_window->draw_lines({
            "Protects Printer_Output",
            "Val " + std::to_string(printer_semaphore.get_sema_value())
                + " | Own "
                + (printer_semaphore.get_owner_task_id() >= 0 ? format_task_id(printer_semaphore.get_owner_task_id()) : "--"),
            "Wait " + std::to_string(printer_semaphore.waiting_task_count())
                + " | "
                + abbreviate_text(printer_semaphore.describe_wait_queue(), panel_text_limit(current_layout.class_width_middle)),
            "d" + std::to_string(printer_semaphore.get_down_operations())
                + " u" + std::to_string(printer_semaphore.get_up_operations())
                + " c" + std::to_string(printer_semaphore.get_contention_events()),
            "Last " + abbreviate_text(compact_semaphore_transition(), panel_text_limit(current_layout.class_width_middle))
        });
        return;
    }

    std::ostringstream value_stream;
    value_stream << "Value: " << printer_semaphore.get_sema_value()
                 << (printer_semaphore.get_sema_value() == 1 ? " (AVAILABLE)" : " (BUSY)");

    semaphore_window->draw_lines({
        fit_panel_text("Purpose: protect Printer_Output and", current_layout.class_width_middle),
        fit_panel_text("serialize access to the critical section.", current_layout.class_width_middle),
        fit_prefixed_panel_text("Resource: ", printer_semaphore.get_resource_name(), current_layout.class_width_middle),
        fit_prefixed_panel_text("Value: ", value_stream.str().substr(7), current_layout.class_width_middle),
        fit_prefixed_panel_text("Owner: ", format_owner_label(), current_layout.class_width_middle)
    });
}

void render_state_panel() {
    if (state_window == nullptr) {
        return;
    }

    if (use_compact_panels()) {
        state_window->draw_lines({
            "READY>RUN>BLK>READY>DEAD",
            abbreviate_text(compact_memory_usage(), panel_text_limit(current_layout.class_width_right)),
            abbreviate_text(memory_buffer_summary(), panel_text_limit(current_layout.class_width_right)),
            "Mem: " + abbreviate_text(latest_memory_event, panel_text_limit(current_layout.class_width_right) - 5),
            abbreviate_text(latest_flow_summary, panel_text_limit(current_layout.class_width_right))
        });
        return;
    }

    state_window->draw_lines({
        fit_panel_text("State legend: READY -> RUNNING -> BLOCKED ->", current_layout.class_width_right),
        fit_panel_text("READY -> DEAD | Race proof: pthread mutex serialized ncurses writes.", current_layout.class_width_right),
        fit_prefixed_panel_text("Memory: ", memory_usage_summary(), current_layout.class_width_right),
        fit_prefixed_panel_text("Buffers: ", memory_buffer_summary(), current_layout.class_width_right),
        fit_prefixed_panel_text("Latest: ", latest_memory_event, current_layout.class_width_right)
    });
}

std::vector<std::string> build_task_panel_lines(
    const std::vector<TaskSnapshot>& snapshots,
    const std::string& role,
    int task_id,
    const std::vector<std::string>& history,
    int panel_width
) {
    const TaskSnapshot* snapshot = find_snapshot(snapshots, task_id);
    if (use_compact_panels()) {
        return {
            abbreviate_text(role, panel_text_limit(panel_width)),
            "ID " + format_task_id(task_id) + " | " + (snapshot != nullptr ? compact_state_text(snapshot->task_state) : "NEW"),
            abbreviate_text(snapshot != nullptr ? snapshot->detail_note : "Awaiting creation.", panel_text_limit(panel_width)),
            "Latest:",
            abbreviate_text(latest_history_line(history), panel_text_limit(panel_width))
        };
    }

    std::vector<std::string> lines {
        fit_prefixed_panel_text("Role: ", role, panel_width),
        fit_prefixed_panel_text("Task ID: ", format_task_id(task_id), panel_width),
        fit_prefixed_panel_text("State: ", snapshot != nullptr ? state_to_text(snapshot->task_state) : "NOT PRESENT", panel_width),
        fit_prefixed_panel_text("Note: ", snapshot != nullptr ? snapshot->detail_note : "Awaiting creation.", panel_width),
        fit_prefixed_panel_text("Last: ", latest_history_line(history), panel_width)
    };

    return lines;
}

void render_task_panels() {
    if (task_window_a == nullptr || task_window_b == nullptr || task_window_c == nullptr) {
        return;
    }

    const std::vector<TaskSnapshot> snapshots = sys_scheduler.snapshot_tasks();

    task_window_a->draw_lines(build_task_panel_lines(
        snapshots,
        "First claimant / holds Printer_Output for one quantum.",
        task_a_id,
        task_a_history,
        current_layout.task_width_left
    ));
    task_window_b->draw_lines(build_task_panel_lines(
        snapshots,
        "Second claimant / should block behind Task_A.",
        task_b_id,
        task_b_history,
        current_layout.task_width_middle
    ));
    task_window_c->draw_lines(build_task_panel_lines(
        snapshots,
        "Third claimant / should block behind Task_B.",
        task_c_id,
        task_c_history,
        current_layout.task_width_right
    ));
}

void render_console() {
    if (console_window == nullptr) {
        return;
    }

    if (use_compact_panels()) {
        console_window->draw_lines({
            "d dump | h help",
            std::string("p pause | q ") + (stop_after_cycle ? "exit" : "stop"),
            "Cycle " + std::to_string(demo_cycle_number)
                + " | A" + std::to_string(sys_scheduler.get_active_task_count())
                + " W" + std::to_string(printer_semaphore.waiting_task_count())
                + " | " + compact_memory_usage(),
            abbreviate_text(console_status, panel_text_limit(current_layout.console_width))
        });
        return;
    }

    console_window->draw_lines({
        fit_panel_text("Controls: d dump | h help", current_layout.console_width),
        fit_panel_text(std::string("p pause | q ") + (stop_after_cycle ? "exit" : "stop"), current_layout.console_width),
        fit_prefixed_panel_text("Cycle: ", std::to_string(demo_cycle_number), current_layout.console_width),
        fit_panel_text(
            "Paused: " + std::string(demo_paused ? "yes" : "no")
                + " | Stop: " + std::string(stop_after_cycle ? "yes" : "no")
                + " | " + memory_usage_summary(),
            current_layout.console_width
        )
    });
}

void sync_visuals() {
    if (transcript_only_mode || header_window == nullptr) {
        return;
    }

    render_header();
    render_scheduler_panel();
    render_semaphore_panel();
    render_state_panel();
    render_task_panels();
    render_console();
    ui_manager.refresh_all();
}

void set_console_status(const std::string& status_line) {
    console_status = status_line;
    sync_visuals();
}

void log_event(const std::string& message) {
    append_transcript(message);
    write_line(log_window, message);
}

void note_memory_event(const std::string& message) {
    latest_memory_event = message;
    log_event("[MemMgr] " + message);
    add_state_trace("Mem_mgr: " + message);
}

bool reserve_task_buffer(
    const std::string& task_name,
    std::size_t bytes,
    const std::string& label,
    const std::string& payload,
    std::size_t& address
) {
    if (address != Mem_mgr::kInvalidAddress) {
        return true;
    }

    address = proof_memory_manager.allocate(bytes, label);
    if (address == Mem_mgr::kInvalidAddress) {
        note_memory_event("allocation failed for " + task_name + " (" + std::to_string(bytes) + " bytes)");
        return false;
    }

    const std::size_t payload_bytes = std::min(bytes, payload.size());
    if (payload_bytes > 0 && !proof_memory_manager.write(address, payload.data(), payload_bytes)) {
        note_memory_event("write failed for " + task_name + " at " + format_memory_address(address));
        proof_memory_manager.free_block(address);
        address = Mem_mgr::kInvalidAddress;
        return false;
    }

    note_memory_event(
        "allocated " + std::to_string(bytes) + " bytes for "
        + task_name + " at " + format_memory_address(address)
    );
    return true;
}

void release_task_buffer(const std::string& task_name, std::size_t& address) {
    if (address == Mem_mgr::kInvalidAddress) {
        return;
    }

    const std::size_t released_address = address;
    if (proof_memory_manager.free_block(address)) {
        note_memory_event("freed buffer for " + task_name + " at " + format_memory_address(released_address));
    } else {
        note_memory_event("free failed for " + task_name + " at " + format_memory_address(released_address));
    }

    address = Mem_mgr::kInvalidAddress;
}

void visual_pause(int total_ms) {
    if (transcript_only_mode) {
        return;
    }

    int elapsed_ms = 0;
    while (elapsed_ms < total_ms) {
        if (console_window != nullptr) {
            const int input = wgetch(console_window->get_win_ptr());
            if (input != ERR) {
                switch (input) {
                    case 'd':
                    case 'D':
                        log_event("");
                        log_event("--- ON-DEMAND SYSTEM DUMP ---");
                        write_block(log_window, capture_dump_text([] {
                            sys_scheduler.dump(1);
                        }));
                        write_block(log_window, capture_dump_text([] {
                            printer_semaphore.dump(1);
                        }));
                        write_block(log_window, capture_dump_text([] {
                            proof_memory_manager.dump(1);
                        }));
                        log_event("");
                        console_status = "Manual dump captured.";
                        sync_visuals();
                        break;
                    case 'h':
                    case 'H':
                        console_status = "Help refreshed.";
                        sync_visuals();
                        break;
                    case 'p':
                    case 'P':
                        demo_paused = !demo_paused;
                        console_status = demo_paused ? "Scheduler paused for inspection." : "Scheduler resumed.";
                        sync_visuals();
                        break;
                    case 'q':
                    case 'Q':
                        stop_after_cycle = true;
                        console_status = "Continuous demo will stop after the current cycle.";
                        sync_visuals();
                        break;
                    case KEY_MOUSE: {
                        MEVENT event {};
                        if (getmouse(&event) == OK) {
                            std::ostringstream mouse_line;
                            mouse_line << "[Mouse] row=" << event.y << ", col=" << event.x;
                            log_event(mouse_line.str());
                            console_status = "Mouse event logged.";
                            sync_visuals();
                        }
                        break;
                    }
                    default:
                        break;
                }
            }
        }

        if (demo_paused) {
            napms(120);
            continue;
        }

        const int slice_ms = std::min(120, total_ms - elapsed_ms);
        napms(slice_ms);
        elapsed_ms += slice_ms;
    }
}

void pace_step(const std::string& status_line, int pause_ms = kStepPauseMs) {
    console_status = status_line;
    sync_visuals();
    visual_pause(pause_ms);
}

void note_task_a(const std::string& line) {
    add_task_history(task_a_history, line);
    sync_visuals();
}

void note_task_b(const std::string& line) {
    add_task_history(task_b_history, line);
    sync_visuals();
}

void note_task_c(const std::string& line) {
    add_task_history(task_c_history, line);
    sync_visuals();
}

void show_system_snapshot(const std::string& title) {
    log_event("");
    log_event(title);

    const std::string scheduler_dump = capture_dump_text([] {
        sys_scheduler.dump(1);
    });
    append_transcript(scheduler_dump);
    write_block(log_window, scheduler_dump);

    const std::string semaphore_dump = capture_dump_text([] {
        printer_semaphore.dump(1);
    });
    append_transcript(semaphore_dump);
    write_block(log_window, semaphore_dump);

    const std::string memory_dump = capture_dump_text([] {
        proof_memory_manager.dump(1);
    });
    append_transcript(memory_dump);
    write_block(log_window, memory_dump);
    log_event("");

    sync_visuals();
    visual_pause(kDumpPauseMs);
}

bool current_task_is_blocked() {
    const int current_task_id = sys_scheduler.get_current_task_id();
    return current_task_id >= 0 && sys_scheduler.is_task_blocked(current_task_id);
}

void reset_cycle_state() {
    sys_scheduler.reset();
    printer_semaphore.reset();
    proof_memory_manager.reset();

    transcript_lines.clear();
    state_trace_lines.clear();
    task_a_history.clear();
    task_b_history.clear();
    task_c_history.clear();

    task_a_id = -1;
    task_b_id = -1;
    task_c_id = -1;

    task_a_phase = 0;
    task_b_phase = 0;
    task_c_phase = 0;

    task_b_was_blocked = false;
    task_c_was_blocked = false;
    mid_execution_dump_written = false;
    post_release_dump_written = false;

    task_a_buffer_address = Mem_mgr::kInvalidAddress;
    task_b_buffer_address = Mem_mgr::kInvalidAddress;
    task_c_buffer_address = Mem_mgr::kInvalidAddress;

    latest_flow_summary = "Printer_Output is free and the process table is empty.";
    latest_memory_event = "Heap reset and ready for deterministic first-fit allocations.";
    console_status = "Preparing cycle " + std::to_string(demo_cycle_number) + ".";

    add_state_trace("Cycle " + std::to_string(demo_cycle_number) + ": scheduler reset; Printer_Output unlocked.");
    add_state_trace("Cycle " + std::to_string(demo_cycle_number) + ": Mem_mgr reset to one free segment.");

    if (!transcript_only_mode) {
        ui_manager.clear_all();
    }
}

void create_demo_tasks() {
    task_a_id = sys_scheduler.create_task("Task_A", [] {
        switch (task_a_phase) {
            case 0: {
                sys_scheduler.set_task_note(task_a_id, "Reserving a work buffer and attempting to claim Printer_Output.");
                if (!reserve_task_buffer(
                        "Task_A",
                        kTaskABufferBytes,
                        "Task_A_work",
                        "Task_A acquired the first work buffer.",
                        task_a_buffer_address
                    )) {
                    sys_scheduler.set_task_note(task_a_id, "Mem_mgr could not reserve Task_A's work buffer.");
                    note_task_a("Mem_mgr allocation failed before Task_A could enter the critical section.");
                    latest_flow_summary = "Task_A could not allocate its work buffer.";
                    sys_scheduler.kill_task(task_a_id);
                    task_a_phase = 2;
                    return;
                }
                note_task_a(
                    "Mem_mgr reserved " + std::to_string(kTaskABufferBytes)
                    + " bytes at " + format_memory_address(task_a_buffer_address) + "."
                );
                note_task_a("Calling down(Printer_Output).");
                log_event("[Task_A] down(Printer_Output)");
                add_state_trace("Task_A moved READY -> RUNNING and requested Printer_Output.");
                latest_flow_summary = "Task_A is the first claimant and should acquire Printer_Output.";
                pace_step("Task_A is requesting Printer_Output.");

                printer_semaphore.down();
                task_a_phase = 1;

                if (current_task_is_blocked()) {
                    sys_scheduler.set_task_note(task_a_id, "Unexpected BLOCKED state detected.");
                    note_task_a("Unexpected BLOCKED state encountered.");
                    return;
                }

                sys_scheduler.set_task_note(task_a_id, "Holding Printer_Output and Task_A's work buffer inside the critical section.");
                note_task_a("Acquired Printer_Output and now owns the critical section.");
                note_task_a("Yielding once so Task_B and Task_C can contend for the resource.");
                log_event("[Task_A] acquired Printer_Output.");
                add_state_trace("Task_A now owns Printer_Output and yields while keeping the resource.");
                latest_flow_summary = "Task_A acquired Printer_Output; Task_B and Task_C must block if they contend.";
                pace_step("Task_A is RUNNING and holding Printer_Output.");
                return;
            }
            case 1:
                sys_scheduler.set_task_note(task_a_id, "Releasing Printer_Output, freeing the work buffer, and preparing to exit.");
                note_task_a("Calling up(Printer_Output) to release the resource.");
                log_event("[Task_A] up(Printer_Output)");
                add_state_trace("Task_A is releasing Printer_Output and waking the next waiter.");
                pace_step("Task_A is releasing Printer_Output.");

                printer_semaphore.up();
                release_task_buffer("Task_A", task_a_buffer_address);
                note_task_a("Freed Task_A's work buffer after the critical section.");
                latest_flow_summary = "Task_A released Printer_Output; Task_B is the next READY owner.";

                if (!post_release_dump_written) {
                    show_system_snapshot("--- POST-RELEASE STATE DUMP ---");
                    post_release_dump_written = true;
                }

                sys_scheduler.kill_task(task_a_id);
                sys_scheduler.set_task_note(task_a_id, "DEAD after completing the first critical section.");
                note_task_a("kill_task() moved Task_A into DEAD.");
                log_event("[Task_A] marked DEAD.");
                add_state_trace("Task_A transitioned to DEAD.");
                latest_flow_summary = "Task_A is DEAD; the scheduler will now rotate through the awakened tasks.";
                pace_step("Task_A finished and entered DEAD.");
                task_a_phase = 2;
                return;
            default:
                return;
        }
    });

    task_b_id = sys_scheduler.create_task("Task_B", [] {
        switch (task_b_phase) {
            case 0: {
                sys_scheduler.set_task_note(task_b_id, "Reserving a wait-record buffer and attempting to claim Printer_Output behind Task_A.");
                if (!reserve_task_buffer(
                        "Task_B",
                        kTaskBBufferBytes,
                        "Task_B_wait_record",
                        "Task_B is queued behind Task_A.",
                        task_b_buffer_address
                    )) {
                    sys_scheduler.set_task_note(task_b_id, "Mem_mgr could not reserve Task_B's wait-record buffer.");
                    note_task_b("Mem_mgr allocation failed before Task_B could contend for Printer_Output.");
                    latest_flow_summary = "Task_B could not allocate its wait-record buffer.";
                    sys_scheduler.kill_task(task_b_id);
                    task_b_phase = 3;
                    return;
                }
                note_task_b(
                    "Mem_mgr reserved " + std::to_string(kTaskBBufferBytes)
                    + " bytes at " + format_memory_address(task_b_buffer_address) + "."
                );
                note_task_b("Calling down(Printer_Output).");
                log_event("[Task_B] down(Printer_Output)");
                add_state_trace("Task_B moved READY -> RUNNING and contended for Printer_Output.");
                latest_flow_summary = "Task_B is testing contention against Task_A.";
                pace_step("Task_B is requesting Printer_Output.");

                printer_semaphore.down();
                task_b_phase = 1;

                if (current_task_is_blocked()) {
                    task_b_was_blocked = true;
                    sys_scheduler.set_task_note(task_b_id, "BLOCKED behind Task_A in the semaphore queue.");
                    note_task_b("BLOCKED and placed in the FIFO wait queue.");
                    log_event("[Task_B] transitioned to BLOCKED.");
                    add_state_trace("Task_B transitioned RUNNING -> BLOCKED.");
                    latest_flow_summary = "Task_A -> Task_B BLOCKED";
                    pace_step("Task_B is BLOCKED behind Task_A.");
                    return;
                }
            }
                [[fallthrough]];
            case 1:
                if (task_b_was_blocked) {
                    sys_scheduler.set_task_note(task_b_id, "Woken from the semaphore queue and ready to run.");
                    note_task_b("Woken from the semaphore queue and returned to READY.");
                    add_state_trace("Task_B transitioned BLOCKED -> READY -> RUNNING.");
                }

                sys_scheduler.set_task_note(task_b_id, "Holding Printer_Output after the FIFO wake-up.");
                note_task_b("Acquired Printer_Output after Task_A released it.");
                log_event("[Task_B] acquired Printer_Output.");
                latest_flow_summary = "Task_B owns Printer_Output after Task_A.";
                pace_step("Task_B is RUNNING with Printer_Output.");
                task_b_phase = 2;
                return;
            case 2:
                sys_scheduler.set_task_note(task_b_id, "Releasing Printer_Output, freeing the wait-record buffer, and preparing to exit.");
                note_task_b("Calling up(Printer_Output).");
                log_event("[Task_B] up(Printer_Output)");
                add_state_trace("Task_B released Printer_Output and should wake Task_C.");
                pace_step("Task_B is releasing Printer_Output.");

                printer_semaphore.up();
                release_task_buffer("Task_B", task_b_buffer_address);
                note_task_b("Freed Task_B's wait-record buffer.");
                sys_scheduler.kill_task(task_b_id);
                sys_scheduler.set_task_note(task_b_id, "DEAD after finishing the second critical section.");
                note_task_b("kill_task() moved Task_B into DEAD.");
                log_event("[Task_B] marked DEAD.");
                add_state_trace("Task_B transitioned to DEAD.");
                latest_flow_summary = "Task_B is DEAD; Task_C is next to acquire Printer_Output.";
                pace_step("Task_B finished and entered DEAD.");
                task_b_phase = 3;
                return;
            default:
                return;
        }
    });

    task_c_id = sys_scheduler.create_task("Task_C", [] {
        switch (task_c_phase) {
            case 0: {
                sys_scheduler.set_task_note(task_c_id, "Reserving a wait-record buffer and attempting to claim Printer_Output behind Task_B.");
                if (!reserve_task_buffer(
                        "Task_C",
                        kTaskCBufferBytes,
                        "Task_C_wait_record",
                        "Task_C is queued behind Task_B.",
                        task_c_buffer_address
                    )) {
                    sys_scheduler.set_task_note(task_c_id, "Mem_mgr could not reserve Task_C's wait-record buffer.");
                    note_task_c("Mem_mgr allocation failed before Task_C could contend for Printer_Output.");
                    latest_flow_summary = "Task_C could not allocate its wait-record buffer.";
                    sys_scheduler.kill_task(task_c_id);
                    task_c_phase = 3;
                    return;
                }
                note_task_c(
                    "Mem_mgr reserved " + std::to_string(kTaskCBufferBytes)
                    + " bytes at " + format_memory_address(task_c_buffer_address) + "."
                );
                note_task_c("Calling down(Printer_Output).");
                log_event("[Task_C] down(Printer_Output)");
                add_state_trace("Task_C moved READY -> RUNNING and contended for Printer_Output.");
                latest_flow_summary = "Task_C is contending after Task_B.";
                pace_step("Task_C is requesting Printer_Output.");

                printer_semaphore.down();
                task_c_phase = 1;

                if (current_task_is_blocked()) {
                    task_c_was_blocked = true;
                    sys_scheduler.set_task_note(task_c_id, "BLOCKED behind Task_B in the semaphore queue.");
                    note_task_c("BLOCKED and placed in the FIFO wait queue.");
                    log_event("[Task_C] transitioned to BLOCKED.");
                    add_state_trace("Task_C transitioned RUNNING -> BLOCKED.");
                    latest_flow_summary = "Task_A -> Task_B BLOCKED -> Task_C BLOCKED";
                    log_event("FLOW: Task_A -> Task_B BLOCKED -> Task_C BLOCKED");
                    pace_step("Task_C is BLOCKED behind Task_B.");

                    if (!mid_execution_dump_written) {
                        show_system_snapshot("--- MID-EXECUTION STATE DUMP (Proving Queueing) ---");
                        mid_execution_dump_written = true;
                    }
                    return;
                }
            }
                [[fallthrough]];
            case 1:
                if (task_c_was_blocked) {
                    sys_scheduler.set_task_note(task_c_id, "Woken from the semaphore queue and ready to run.");
                    note_task_c("Woken from the semaphore queue and returned to READY.");
                    add_state_trace("Task_C transitioned BLOCKED -> READY -> RUNNING.");
                }

                sys_scheduler.set_task_note(task_c_id, "Holding Printer_Output after the final FIFO wake-up.");
                note_task_c("Acquired Printer_Output after Task_B released it.");
                log_event("[Task_C] acquired Printer_Output.");
                latest_flow_summary = "Task_C owns Printer_Output after the queued wake-up.";
                pace_step("Task_C is RUNNING with Printer_Output.");
                task_c_phase = 2;
                return;
            case 2:
                sys_scheduler.set_task_note(task_c_id, "Releasing Printer_Output, freeing the wait-record buffer, and preparing to exit.");
                note_task_c("Calling up(Printer_Output).");
                log_event("[Task_C] up(Printer_Output)");
                add_state_trace("Task_C released Printer_Output and will exit.");
                pace_step("Task_C is releasing Printer_Output.");

                printer_semaphore.up();
                release_task_buffer("Task_C", task_c_buffer_address);
                note_task_c("Freed Task_C's wait-record buffer.");
                sys_scheduler.kill_task(task_c_id);
                sys_scheduler.set_task_note(task_c_id, "DEAD after finishing the final critical section.");
                note_task_c("kill_task() moved Task_C into DEAD.");
                log_event("[Task_C] marked DEAD.");
                add_state_trace("Task_C transitioned to DEAD.");
                latest_flow_summary = "All work completed; all three tasks have reached DEAD.";
                pace_step("Task_C finished and entered DEAD.");
                task_c_phase = 3;
                return;
            default:
                return;
        }
    });

    sys_scheduler.set_task_note(task_a_id, "READY: first claimant that should acquire Printer_Output.");
    sys_scheduler.set_task_note(task_b_id, "READY: second claimant that should block behind Task_A.");
    sys_scheduler.set_task_note(task_c_id, "READY: third claimant that should block behind Task_B.");

    add_task_history(task_a_history, "Created in READY state.");
    add_task_history(task_a_history, "Will acquire Printer_Output first and yield while holding it.");
    add_task_history(task_a_history, "Mem_mgr will reserve a work buffer before the critical section.");
    add_task_history(task_b_history, "Created in READY state.");
    add_task_history(task_b_history, "Should block when Task_A still owns Printer_Output.");
    add_task_history(task_b_history, "Mem_mgr will reserve a wait-record buffer before blocking.");
    add_task_history(task_c_history, "Created in READY state.");
    add_task_history(task_c_history, "Should block behind Task_B in the FIFO queue.");
    add_task_history(task_c_history, "Mem_mgr will reserve a wait-record buffer before blocking.");

    add_state_trace("Three tasks were inserted into the process table.");
    add_state_trace("The scheduler will rotate Task_A -> Task_B -> Task_C by READY order.");
    add_state_trace("Mem_mgr will track one buffer per task through acquire, block, wake, and release.");
}

void create_windows() {
    task_window_a = nullptr;
    task_window_b = nullptr;
    task_window_c = nullptr;
    log_window = nullptr;
    console_window = nullptr;

    header_window = new U2_window(
        current_layout.header_height,
        current_layout.header_width,
        current_layout.header_y,
        current_layout.header_x,
        "ULTIMA 2.0 - Phase 1",
        false
    );
    scheduler_window = new U2_window(
        current_layout.class_height,
        current_layout.class_width_left,
        current_layout.class_y,
        current_layout.class_x_left,
        "Scheduler Class",
        false
    );
    semaphore_window = new U2_window(
        current_layout.full_width_panels ? current_layout.class_height : (current_layout.stacked_primary_layout ? current_layout.task_height : current_layout.class_height),
        current_layout.class_width_middle,
        current_layout.full_width_panels ? current_layout.semaphore_y : (current_layout.stacked_primary_layout ? current_layout.task_y : current_layout.class_y),
        current_layout.class_x_middle,
        "Semaphore Class",
        false
    );
    state_window = new U2_window(
        current_layout.full_width_panels ? current_layout.state_height : (current_layout.stacked_primary_layout ? current_layout.bottom_height : current_layout.class_height),
        current_layout.class_width_right,
        current_layout.full_width_panels ? current_layout.state_y : (current_layout.stacked_primary_layout ? current_layout.bottom_y : current_layout.class_y),
        current_layout.class_x_right,
        "State + Race + Memory",
        false
    );

    if (current_layout.full_width_panels) {
        task_window_a = new U2_window(
            current_layout.task_height,
            current_layout.task_width_left,
            current_layout.task_a_y,
            current_layout.task_x_left,
            "Task_A",
            false
        );
        task_window_b = new U2_window(
            current_layout.task_height,
            current_layout.task_width_middle,
            current_layout.task_b_y,
            current_layout.task_x_middle,
            "Task_B",
            false
        );
        task_window_c = new U2_window(
            current_layout.task_height,
            current_layout.task_width_right,
            current_layout.task_c_y,
            current_layout.task_x_right,
            "Task_C",
            false
        );
        log_window = new U2_window(
            current_layout.log_height,
            current_layout.log_width,
            current_layout.log_y,
            current_layout.log_x,
            "Output Log + Dumps",
            true
        );
        console_window = new U2_window(
            current_layout.console_height,
            current_layout.console_width,
            current_layout.console_y,
            current_layout.console_x,
            "Controls",
            false
        );
    } else if (!current_layout.stacked_primary_layout) {
        task_window_a = new U2_window(
            current_layout.task_height,
            current_layout.task_width_left,
            current_layout.task_y,
            current_layout.task_x_left,
            "Task_A",
            false
        );
        task_window_b = new U2_window(
            current_layout.task_height,
            current_layout.task_width_middle,
            current_layout.task_y,
            current_layout.task_x_middle,
            "Task_B",
            false
        );
        task_window_c = new U2_window(
            current_layout.task_height,
            current_layout.task_width_right,
            current_layout.task_y,
            current_layout.task_x_right,
            "Task_C",
            false
        );
        log_window = new U2_window(
            current_layout.bottom_height,
            current_layout.log_width,
            current_layout.bottom_y,
            current_layout.log_x,
            "Output Log + Dumps",
            true
        );
        console_window = new U2_window(
            current_layout.bottom_height,
            current_layout.console_width,
            current_layout.bottom_y,
            current_layout.console_x,
            "Controls",
            false
        );
    }

    ui_manager.add_window(header_window);
    ui_manager.add_window(scheduler_window);
    ui_manager.add_window(semaphore_window);
    ui_manager.add_window(state_window);
    ui_manager.add_window(task_window_a);
    ui_manager.add_window(task_window_b);
    ui_manager.add_window(task_window_c);
    ui_manager.add_window(log_window);
    ui_manager.add_window(console_window);

    if (console_window != nullptr) {
        keypad(console_window->get_win_ptr(), TRUE);
        nodelay(console_window->get_win_ptr(), TRUE);
        mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, nullptr);
        mouseinterval(0);
    }
}

void run_demo_cycle() {
    reset_cycle_state();
    create_demo_tasks();

    sync_visuals();

    log_event("=== ULTIMA Phase 1 terminal demonstration / cycle " + std::to_string(demo_cycle_number) + " ===");
    log_event("Scheduler purpose: manage the dynamic process table and choose the next READY task to run.");
    log_event("Semaphore purpose: prevent a shared Printer_Output race by blocking and queueing contenders.");
    log_event("Memory manager purpose: provide deterministic first-fit task buffers and show their lifetime in the proof board.");
    log_event("Pthread purpose: protect ncurses writes so the proof-board display stays coherent.");
    if (stop_after_cycle) {
        log_event("Single-cycle mode: the final state will remain on screen when the demo finishes.");
    } else {
        log_event("Continuous mode: the interactive demo will restart automatically until you press q.");
    }
    show_system_snapshot("--- INITIAL SYSTEM STATE ---");
    log_event("--- BEGINNING EXECUTION ---");
    pace_step("Scheduler run started.");

    while (sys_scheduler.has_active_tasks()) {
        sys_scheduler.yield();
        sync_visuals();
        visual_pause(kDispatchPauseMs);
    }

    show_system_snapshot("--- FINAL STATE DUMP (Before Garbage Collection) ---");
    sys_scheduler.garbage_collect();
    sync_visuals();
    show_system_snapshot("--- POST-GARBAGE-COLLECTION DUMP ---");
    log_event("System shutting down safely.");
    add_state_trace("Cycle proof completed: tasks are DEAD and the process table has been collected.");
    latest_flow_summary = "Cycle complete. The scheduler, semaphore, and Mem_mgr proved task state, FIFO wake-up, and buffer lifetime.";
    pace_step("Cycle complete. Final state is now visible on the proof board.", 1200);
}

} // namespace

int main(int argc, char* argv[]) {
    std::string output_file_path;

    for (int index = 1; index < argc; ++index) {
        const std::string argument = argv[index];
        if (argument == "--transcript-only") {
            transcript_only_mode = true;
            continue;
        }

        if (argument == "--no-post-ui-transcript") {
            auto_print_transcript_after_ui = false;
            continue;
        }

        if (argument == "--post-ui-transcript") {
            auto_print_transcript_after_ui = true;
            continue;
        }

        if (argument == "--continuous") {
            continuous_demo_requested = true;
            continue;
        }

        if (argument == "--output-file") {
            if (index + 1 >= argc) {
                std::cerr << "Missing path after --output-file" << std::endl;
                return 1;
            }
            output_file_path = argv[++index];
            continue;
        }
    }

    if (!transcript_only_mode && output_file_path.empty()) {
        const std::string cwd = current_working_directory();
        if (!cwd.empty()) {
            output_file_path = join_path(cwd, "phase1output.txt");
        }
    }

#if defined(SIGHUP)
    std::signal(SIGHUP, SIG_IGN);
#endif

    #if !defined(ULTIMA_ENABLE_CURSES)
    if (!transcript_only_mode) {
        if (try_launch_sibling_curses_binary(argc, argv)) {
            return 0;
        }
        const std::string binary_directory = resolve_invoked_binary_directory(argv);
        if (!binary_directory.empty()
            && try_rebuild_native_curses_binary(binary_directory)
            && try_launch_sibling_curses_binary(argc, argv)) {
            return 0;
        }
        transcript_only_mode = true;
        std::cerr << "This Ultima binary was built without ncurses support. "
                  << "Rebuild it with the repository Makefile or CMake target "
                  << "to open the multi-window Phase 1 UI."
                  << std::endl;
    }
    #endif

    #if defined(ULTIMA_ENABLE_CURSES) && defined(__APPLE__)
    if (!transcript_only_mode && !terminal_program_is_known_good() && running_inside_jetbrains_debugger()) {
        if (try_open_external_terminal_launcher(argv)) {
            std::cerr << "Opening Terminal.app because the current JetBrains debug console does not render ncurses windows."
                      << std::endl;
            return 0;
        }
    }
    #endif

    #if defined(ULTIMA_ENABLE_CURSES)
    if (!transcript_only_mode && !environment_supports_curses_ui()) {
        transcript_only_mode = true;
        std::cerr << "Current console does not provide a usable ncurses terminal surface. "
                  << "Falling back to transcript-only mode. Run Ultima from Terminal/iTerm, "
                  << "or use the macOS .command launcher, to see the multi-window UI."
                  << std::endl;
    }

    if (!transcript_only_mode) {
        request_preferred_terminal_size_if_needed();
        ui_manager.init_ncurses_env();

        if (!build_layout(current_layout)) {
            ui_manager.close_ncurses_env();
            transcript_only_mode = true;
            std::cerr << "Terminal surface is smaller than "
                      << kMinimumTerminalCols
                      << " columns by "
                      << kMinimumTerminalRows
                      << " rows; falling back to transcript-only mode."
                      << std::endl;
        }

        if (!transcript_only_mode) {
            create_windows();
            sync_visuals();
            stop_after_cycle = !continuous_demo_requested;
        }
    }

    do {
        run_demo_cycle();
        if (transcript_only_mode || stop_after_cycle) {
            break;
        }

        ++demo_cycle_number;
        latest_flow_summary = "Restarting the continuous scheduler demonstration.";
        pace_step("Restarting the continuous scheduler demonstration.", kCycleRestartPauseMs);
        if (stop_after_cycle) {
            break;
        }
    } while (true);

    if (!transcript_only_mode) {
        if (console_window != nullptr) {
            if (stop_after_cycle && auto_print_transcript_after_ui) {
                nodelay(console_window->get_win_ptr(), FALSE);
                set_console_status("Cycle complete. Press any key to show the transcript.");
                wgetch(console_window->get_win_ptr());
            } else {
                nodelay(console_window->get_win_ptr(), FALSE);
                set_console_status(stop_after_cycle
                    ? "Cycle complete. Press any key to exit."
                    : "Continuous scheduler stopped. Press any key to exit.");
                wgetch(console_window->get_win_ptr());
            }
        }
        ui_manager.close_ncurses_env();
    }
    #endif

    const std::string transcript_text = build_transcript_text();
    const bool print_transcript_to_stdout = transcript_only_mode
        || (!transcript_only_mode && stop_after_cycle && auto_print_transcript_after_ui);

    if (!output_file_path.empty()) {
        std::ofstream output_file(output_file_path);
        if (!output_file.is_open()) {
            std::cerr << "Unable to write transcript file: " << output_file_path << std::endl;
            return 1;
        }
        output_file << transcript_text;
    }

    if (print_transcript_to_stdout) {
        std::cout << transcript_text;
    }

    return 0;
}

#if !defined(ULTIMA_SEPARATE_COMPILATION)
/*
 * Support direct one-file builds. One example command is
 * c++ "/path/to/Ultima.cpp" -o ultima_single
 * The repository build entrypoints define ULTIMA_SEPARATE_COMPILATION=1 and
 * compile these modules as independent translation units instead.
 */
#include "Sched.cpp"
#include "Mem_mgr.cpp"
#include "Sema.cpp"
#include "U2_UI.cpp"
#include "U2_Window.cpp"
#endif
