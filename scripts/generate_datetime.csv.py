import csv
import random
import os
from datetime import datetime, timedelta

output_file = "./temp/datetime_data.csv"

total_rows = 6
total_value_cols = 4

random_min = -100
random_max = 100

start_datetime = datetime(2025, 1, 1, 0, 0, 0)

os.makedirs(os.path.dirname(output_file), exist_ok=True)

with open(output_file, "w", newline="") as file:
    writer = csv.writer(file)

    headers = ["datetime"] + [
        f"value_{i}"
        for i in range(1, total_value_cols + 1)
    ]
    writer.writerow(headers)

    for i in range(total_rows):
        dt = start_datetime + timedelta(days=i * 7)

        row = [dt.strftime("%Y-%m-%d %H:%M:%S")]

        for _ in range(total_value_cols):
            row.append(random.randint(random_min, random_max))

        writer.writerow(row)

print(f"CSV file created: {output_file}")
