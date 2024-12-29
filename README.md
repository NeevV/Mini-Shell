# Mini-Shell
A replica of the bash shell, implemented in the C programming language, designed to enhance understanding of systems programming and shell functionalities.

Features:
- Basic Shell Implementation
Set up the basic shell interface.
Read user input and parse commands.
Implement command execution using fork() and exec() system calls.
Handle simple built-in commands like cd, exit, and pwd.
- Advanced Features
Implement input and output redirection (>, <).
Add support for command pipelines (|).
Allow users to combine redirection with multiple commands in a pipeline.
- Job Control
Implement job control commands like & for background execution, jobs to list running jobs, and fg/bg to bring jobs to the foreground or send them to the background.
Handle process termination (SIGINT/SIGTERM) and job suspension (SIGTSTP).
- Signal Handling and Error Management
Implement proper signal handling for user interruptions (CTRL+C, CTRL+Z) without crashing the shell.
Add error messages for command execution failures (e.g., file not found, permission denied).
Handle corner cases, such as when redirection or pipes are used incorrectly.
- History, Aliases, and Scripting
Implement command history using up/down arrow keys.
Add support for aliases to simplify frequent command usage.
Implement scripting by allowing the shell to read and execute commands from a file (like a .sh script).
