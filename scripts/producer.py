import csv
import os
import random
import time

# Configure multiple pipe files
pipe_files = [
    "/tmp/pipe1",
    "/tmp/pipe2",
]

total_cols = 2

random_min = -100
random_max = 100

increase_min = -10
increase_max = 10

interval_seconds = 0.25


def create_pipe(pipe_path):
    if not os.path.exists(pipe_path):
        os.mkfifo(pipe_path)


# Create all pipes
for pipe_file in pipe_files:
    create_pipe(pipe_file)

print("Waiting for readers to connect...")

# Open all pipes for writing
writers = []
states = []

for pipe_file in pipe_files:
    pipe = open(pipe_file, "w", newline="")
    writer = csv.writer(pipe)

    # Optional header
    # headers = [f"number_{col}" for col in range(1, total_cols + 1)]
    # writer.writerow(headers)
    # pipe.flush()

    writers.append((pipe, writer))

    # Independent random state per pipe
    states.append(
        [
            random.randint(random_min, random_max)
            for _ in range(total_cols - 1)
        ]
    )

row_id = 1

while True:
    for pipe_index, (pipe, writer) in enumerate(writers):
        row = [row_id]

        current_values = states[pipe_index]

        for col_index in range(total_cols - 1):
            current_values[col_index] += random.randint(
                increase_min,
                increase_max
            )
            row.append(current_values[col_index])

        writer.writerow(row)
        pipe.flush()

    row_id += 1
    time.sleep(interval_seconds)
