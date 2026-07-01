import datetime
import os
import random
import csv


def generate_large_csv(
    output_file: str,
    target_size_gb: float,
    total_cols: int = 5,
    chunk_size: int = 100_000,
    timestamp_format: str = "string",  # Accepts "string" or "epoch_ms"
    start_spacing: int = 0             # 0 = same start value, 1000 = spaced by 1000
):
    """
    Generates a CSV file of a specific size with a time column and random numeric columns.

    :param output_file: Path to the output CSV file.
    :param target_size_gb: Target size of the file in Gigabytes.
    :param total_cols: Total number of columns (1 time + N numeric).
    :param chunk_size: Number of rows to write to memory before flushing to disk.
    :param timestamp_format: "string" for 'YYYY-MM-DD HH:MM:SS', "epoch_ms" for integer epoch.
    :param start_spacing: The spacing between the initial values of each numeric column.
    """
    # Convert GB to bytes for file size checking
    target_size_bytes = target_size_gb * 1024 * 1024 * 1024
    target_size_mb = target_size_bytes / (1024 * 1024)

    random_min, random_max = -100, 100
    increase_min, increase_max = -10, 10

    # Create parent folder if it does not exist
    os.makedirs(os.path.dirname(output_file), exist_ok=True)

    # Base starting date
    start_dt = datetime.datetime(2026, 1, 1, 0, 0, 0)

    # Configure time tracking based on the requested format
    is_epoch = (timestamp_format == "epoch_ms")
    if is_epoch:
        # Track time as an integer (much faster for millions of iterations)
        current_time_val = int(start_dt.timestamp() * 1000)
        time_step = 1000  # 1 second = 1000 ms
    else:
        # Track time as a datetime object
        current_time_val = start_dt
        time_step = datetime.timedelta(seconds=1)

    # Initialize the numeric columns based on start_spacing
    base_start_value = random.randint(random_min, random_max)
    current_values = [
        base_start_value + (i * start_spacing)
        for i in range(total_cols - 1)
    ]

    print(f"Starting to generate ~{target_size_gb}GB CSV at {output_file}...")
    print(f"Timestamp format: {timestamp_format}")
    print(f"Column start spacing: {start_spacing} (Base start value: {base_start_value})")

    with open(output_file, "w", newline="") as file:
        writer = csv.writer(file)

        # Optional: Generate and write dynamic headers
        # headers = ["timestamp"] + [f"value_{i}" for i in range(1, total_cols)]
        # writer.writerow(headers)

        current_size = 0

        while current_size < target_size_bytes:
            chunk = []

            for _ in range(chunk_size):
                # Advance the time
                current_time_val += time_step

                # Format the first column depending on the mode
                time_col = current_time_val if is_epoch else str(current_time_val)

                # Increase numeric values
                for col_index in range(total_cols - 1):
                    current_values[col_index] += random.randint(increase_min, increase_max)

                # Create the row
                row = [time_col, *current_values]
                chunk.append(row)

            # Write the chunk and flush to disk
            writer.writerows(chunk)
            file.flush()

            # Check actual file size
            current_size = os.path.getsize(output_file)
            current_size_mb = current_size / (1024 * 1024)

            # Dynamic progress output
            print(f"Current size: {current_size_mb:.2f} MB / {target_size_mb:.2f} MB", end="\r")

    print(f"\nFinished! CSV file created: {output_file}")


# ==========================================
# Execution Block
# ==========================================
if __name__ == "__main__":
    # Change format here: "string" or "epoch_ms"
    TIME_FORMAT = "epoch_ms"

    FILE_PATH = "./temp/very_large_dataset_5GB.csv"
    TARGET_SIZE_IN_GB = 5
    COLUMN_COUNT = 21
    COLUMN_SPACING = 10000  # Set to 0 for all columns to start at the exact same value

    generate_large_csv(
        output_file=FILE_PATH,
        target_size_gb=TARGET_SIZE_IN_GB,
        total_cols=COLUMN_COUNT,
        timestamp_format=TIME_FORMAT,
        start_spacing=COLUMN_SPACING
    )
