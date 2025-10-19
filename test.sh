#!/bin/bash

# Simple test script for minish features

echo "--- Building minish ---"
make clean > /dev/null
make

if [ ! -f minish ]; then
    echo "Build failed. Exiting."
    exit 1
fi

echo "--- Starting minish interactive test (Type 'exit' to finish) ---"
echo " "

# We use 'expect'-like input to feed commands to the shell
(
    # 1. Simple command & cd built-in
    echo "echo '1. Current directory:'"
    echo "pwd"
    echo "cd /tmp"
    echo "echo '1. Changed directory to:'"
    echo "pwd"

    # 2. I/O Redirection Test (Overwrite > and Append >>)
    echo "echo '2. Hello World' > test_output.txt"
    echo "echo ' APPEND' >> test_output.txt"
    echo "cat test_output.txt"

    # 3. Piping Test
    echo "echo '3. Pipe Test: This is a test string' | grep test"

    # 4. Background Job Test (&)
    echo "echo '4. Starting background sleep (5 seconds)...'"
    echo "sleep 5 &"
    echo "jobs" # Should show the sleep job
    echo "sleep 1" # Wait for 1 second

    # 5. Foreground Job Test (fg)
    echo "echo '5. Bringing sleep job to foreground...'"
    echo "fg 1" # Wait for sleep 5 to finish

    # 6. Final check (job should be reaped by SIGCHLD)
    echo "echo '6. Checking jobs after fg (should be empty):'"
    echo "jobs"

    # 7. Exit built-in
    echo "exit"

) | ./minish

# Cleanup
rm -f test_output.txt
make clean > /dev/null

echo " "
echo "--- Test Complete. Review output above. ---"
