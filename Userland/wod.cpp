/*
 * Copyright (c) 2020, Ben Wiederhake <BenWiederhake.GitHub@gmx.de>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <AK/LexicalPath.h>
#include <AK/String.h>
#include <LibCore/ArgsParser.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

static pid_t start_cmd(int pipe_fd[2], char** argv);
static void wait_child(pid_t child);

int main(int argc, char** argv)
{
    /*if (pledge("stdio rpath wpath cpath fattr", nullptr) < 0) {
        perror("pledge");
        return 1;
    } FIXME: pledge */
    // FIXME: While at it: unveil?

    if (argc < 3) {
        fprintf(stderr,
            "Usage:\n"
            "        wod <destination> <cmd...>\n"
            "\n"
            "Arguments:\n"
            "        destination Destination file path\n"
            "        cmd         Command whose output shall be written\n"
            "                    to <destination>, but only if different.\n");
        return 1;
    }

    const char* destination = argv[1];
    char** cmd = &argv[2];

    int pipe_fd[2];
    int rc = pipe(pipe_fd);
    if (rc < 0) {
        perror("pipe");
        return 1;
    }

    pid_t child = start_cmd(pipe_fd, cmd);
    if (child < 0) {
        // start_cmd() already generated an error message.
        return 1;
    }

    int rc = close(pipe_fd[1]);

    // FIXME: read to memory or file

    wait_child(child);

    // FIXME: compare with file
    // FIXME: write to file
    // FIXME: error checking everywhere

    return 0;
}

pid_t start_cmd(int pipe_fd[2], char** argv)
{
    pid_t child = fork();
    if (child < 0) {
        perror("fork");
        return -1;
    }
    if (child > 0) {
        return child;
    }

    int sth = close(pipe_fd[0]);
    int foo = dup2(pipe_fd[1], STDOUT_FILENO);

    char* origname = strdup(argv[0]);
    String exe_basename = AK::LexicalPath(argv[0]).basename();
    argv[0] = const_cast<char*>(exe_basename.characters());
    int rc = execvp(argv[0], argv);

    exit(1);
}

static void wait_child(pid_t child)
{
    int child_status;
    pid_t stopped_pid = waitpid(child, &child_status, 0);
    if (stopped_pid < 0) {
        perror("waitpid");
        exit(1);
    }
    ASSERT(stopped_pid == child);

    int exit_code = WEXITSTATUS(child_status);
    if (exit_code) {
        exit(exit_code);
    }
}
