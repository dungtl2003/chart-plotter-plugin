import csv
import random
import os

output_file = "./temp/trendrand.csv"

total_rows = 56
total_cols = 5

random_min = -100
random_max = 100

# How much each value can increase per row
increase_min = -10
increase_max = 10

# Create parent folder if it does not exist
os.makedirs(os.path.dirname(output_file), exist_ok=True)

with open(output_file, "w", newline="") as file:
    writer = csv.writer(file)

    headers = [f"number_{col}" for col in range(1, total_cols + 1)]
    writer.writerow(headers)

    # First value for each random upward-trending column
    current_values = [
        random.randint(random_min, random_max)
        for _ in range(total_cols - 1)
    ]

    for i in range(1, total_rows + 1):
        row = [i]

        for col_index in range(total_cols - 1):
            # Increase first, so every row goes up
            current_values[col_index] += random.randint(increase_min, increase_max)
            row.append(current_values[col_index])

        writer.writerow(row)

print(f"CSV file created: {output_file}")
