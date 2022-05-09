/*
 * @description: 协程测试
 */
#include <stdio.h>
#include <ucontext.h>

int fib_res;
ucontext_t main_ctx, fib_ctx;

char fib_stack[1024 * 32];

void fib() {
    // (1)
    int a0 = 0;
    int a1 = 1;

    while (1) {
        fib_res = a0 + a1;
        a0 = a1;
        a1 = fib_res;

        // send the result to outer env and hand over the right of control.
        swapcontext(&fib_ctx, &main_ctx);  // (b)
        // (3)
    }
}

int main(int argc, char **argv) {
    // initialize fib_ctx with current context.
    getcontext(&fib_ctx);
    fib_ctx.uc_link = 0;  // after fib() returns we exit the thread.
    fib_ctx.uc_stack.ss_sp = fib_stack;  // specific the stack for fib().
    fib_ctx.uc_stack.ss_size = sizeof(fib_stack);
    fib_ctx.uc_stack.ss_flags = 0;
    makecontext(&fib_ctx, fib, 0);  // modify fib_ctx to run fib() without arguments.

    while (1) {
        // pass the right of control to fib() by swap the context.
        swapcontext(&main_ctx, &fib_ctx);  // (a)
        // (2)
        printf("%d\n", fib_res);
        if (fib_res > 200) {
            break;
        }
    }

    return 0;
}