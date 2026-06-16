import csv
import random
import os

output_file = "./temp/testrand.csv"

total_rows = 20
total_cols = 5  # Total number of columns to generate

random_min = 1
random_max = 100

# Create parent folder if it does not exist
os.makedirs(os.path.dirname(output_file), exist_ok=True)

with open(output_file, "w", newline="") as file:
    writer = csv.writer(file)

    # Create headers: number_1, number_2, number_3, ...
    headers = [f"number_{col}" for col in range(1, total_cols + 1)]
    writer.writerow(headers)

    for i in range(1, total_rows + 1):
        row = [i]

        # Fill remaining columns with random numbers
        for _ in range(total_cols - 1):
            row.append(random.randint(random_min, random_max))

        writer.writerow(row)

print(f"CSV file created: {output_file}")
