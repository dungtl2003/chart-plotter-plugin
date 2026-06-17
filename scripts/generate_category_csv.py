import csv
import random
import os

output_file = "./temp/category_data.csv"

total_categories = 6
total_value_cols = 4

random_min = -100
random_max = 100

categories = [
    "Books",
    "Movies",
    "Music",
    "Games",
    "Sports",
    "Travel",
    "Food",
    "Technology",
    "Health",
    "Education",
    "Finance",
    "Fashion"
]

selected_categories = random.sample(categories, total_categories)

os.makedirs(os.path.dirname(output_file), exist_ok=True)

with open(output_file, "w", newline="") as file:
    writer = csv.writer(file)

    headers = ["category"] + [
        f"value_{i}"
        for i in range(1, total_value_cols + 1)
    ]
    writer.writerow(headers)

    for category in selected_categories:
        row = [category]

        for _ in range(total_value_cols):
            row.append(random.randint(random_min, random_max))

        writer.writerow(row)

print(f"CSV file created: {output_file}")
