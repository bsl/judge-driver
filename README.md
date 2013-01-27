From `main.c`:

judge-driver executes a given program under a pseudo terminal session and
serves as a bidirectional proxy, relaying stdin and stdout, allowing the caller
to interact with the child. It also allows the caller to specify the child's
argument list, environment, and restrict the time and memory the child is
allowed to use.

judge-driver expects its argv to be the argv of the child (see `exec(3)`,
`execve`).

Additionally, 3 environment variables must be set:

* `JD_MAX_CPU_TIME`:
  Unsigned integer number of seconds. If the child is still running after
  consuming the given number of seconds of CPU time, it will be killed by the
  kernel and judge-driver will exit.

* `JD_MAX_USER_TIME`:
  Unsigned integer number of seconds. If the child is still running after the
  given number of seconds, judge-driver will kill it and exit. This number
  should almost certainly be larger than `JD_MAX_CPU_TIME`.

* `JD_MAX_MEM`:
  Unsigned integer number of bytes. If the child attempts to allocate more
  memory than this number, it will fail.

By default, the child's environment will be empty. To specify entries, set
variables in judge-driver's environment with prefix `JD_ENV_` like

    JD_ENV_XXX=YYY
    JD_ENV_ABC=123
    JD_ENV_KEY=VALUE

Done this way, the child's environment will be

    XXX=YYY
    ABC=123
    KEY=VALUE

judge-driver should always exit in one of the following situations and exit
codes:

* _40_: the child exited normally
* _41_: the user time limit was exceeded
* _42_: the CPU time limit was exceeded
* _128+n_: the child was killed by unexpected signal _n_
* _50_: incorrect parameters were given to judge-driver
* _51_: an internal error occurred
