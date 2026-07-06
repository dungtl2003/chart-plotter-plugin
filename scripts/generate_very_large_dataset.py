import datetime
import os
import random


def generate_large_csv(
    output_file: str,
    target_size_gb: float,
    total_cols: int = 5,
    chunk_size: int = 100_000,
    timestamp_format: str = "string",  # Accepts "string" or "epoch_ms"
    start_spacing: int = 0,            # 0 = same start value, 1000 = spaced by 1000
    step_min: int = -10,               # smallest per-step change to a value
    step_max: int = 10                 # largest per-step change to a value
):
    """
    Generates a CSV file of a specific size with a time column and random numeric columns.

    Each numeric column is a plain (unbounded) random walk: every row it moves by
    a uniform random integer in [step_min, step_max]. Values are free to drift as
    far as the walk takes them.

    The generator is tuned for throughput rather than doing the obvious per-cell
    work:
      * String timestamps are built from a precomputed 86400-entry "HH:MM:SS"
        table plus a date prefix that is only rebuilt once per day, instead of
        calling str(datetime) on every row.
      * The whole numeric part of a row is formatted in a single C-level printf
        (one "%d,%d,..." over a tuple) rather than per-cell str()/append().
      * Each chunk is joined and written in one call; hot names are hoisted into
        locals outside the millions-of-iterations loop.

    :param output_file: Path to the output CSV file.
    :param target_size_gb: Target size of the file in Gigabytes.
    :param total_cols: Total number of columns (1 time + N numeric).
    :param chunk_size: Number of rows to write to memory before flushing to disk.
    :param timestamp_format: "string" for 'YYYY-MM-DD HH:MM:SS', "epoch_ms" for integer epoch.
    :param start_spacing: The spacing between the initial values of each numeric column.
    :param step_min: Smallest per-step change applied to a value each row.
    :param step_max: Largest per-step change applied to a value each row.
    """
    # Convert GB to bytes for file size checking
    target_size_bytes = target_size_gb * 1024 * 1024 * 1024
    target_size_mb = target_size_bytes / (1024 * 1024)

    random_min, random_max = -100, 100

    # Create parent folder if it does not exist
    os.makedirs(os.path.dirname(output_file), exist_ok=True)

    # Base starting date
    start_dt = datetime.datetime(2026, 1, 1, 0, 0, 0)

    # Configure time tracking based on the requested format
    is_epoch = (timestamp_format == "epoch_ms")
    if is_epoch:
        # Track time as an integer epoch (much faster for millions of rows).
        current_epoch_ms = int(start_dt.timestamp() * 1000)
    else:
        # String timestamps ("YYYY-MM-DD HH:MM:SS"). Calling str(datetime) every
        # row is expensive, so precompute all 86400 "HH:MM:SS" strings once and
        # only rebuild the "YYYY-MM-DD " date prefix when the day rolls over.
        tod_table = [
            "%02d:%02d:%02d" % (t // 3600, (t // 60) % 60, t % 60)
            for t in range(86400)
        ]
        start_date = start_dt.date()
        total_seconds = 0
        day_index = -1
        date_prefix = ""

    ncols = total_cols - 1

    # Initialize the numeric columns based on start_spacing
    base_start_value = random.randint(random_min, random_max)
    current_values = [
        base_start_value + (i * start_spacing)
        for i in range(ncols)
    ]

    # Hoisted hot locals for the inner loop (attribute/constant lookups moved
    # out of the millions-of-iterations path). A step is drawn as
    # int(rnd() * step_span) + step_min, a uniform integer in [step_min, step_max]
    # that is cheaper than random.randint().
    rnd = random.random
    step_span = step_max - step_min + 1
    col_range = range(ncols)
    # Format the whole numeric part of a row in one C-level printf.
    numfmt = "," + ",".join(["%d"] * ncols)

    print(f"Starting to generate ~{target_size_gb}GB CSV at {output_file}...")
    print(f"Timestamp format: {timestamp_format}")
    print(f"Column start spacing: {start_spacing} (Base start value: {base_start_value})")

    with open(output_file, "w", newline="") as file:
        write = file.write

        # Optional: Generate and write dynamic headers
        # headers = ["timestamp"] + [f"value_{i}" for i in range(1, total_cols)]
        # write(",".join(headers) + "\n")

        current_size = 0

        while current_size < target_size_bytes:
            out = []
            append = out.append

            for _ in range(chunk_size):
                # Advance the time and build the timestamp cell.
                if is_epoch:
                    current_epoch_ms += 1000
                    time_col = str(current_epoch_ms)
                else:
                    total_seconds += 1
                    di = total_seconds // 86400
                    if di != day_index:
                        day_index = di
                        date_prefix = (
                            start_date + datetime.timedelta(days=di)
                        ).isoformat() + " "
                    time_col = date_prefix + tod_table[total_seconds % 86400]

                # Advance each numeric column by a uniform random integer step.
                for i in col_range:
                    current_values[i] += int(rnd() * step_span) + step_min

                # Format the whole numeric row in one printf and append it.
                append(time_col + numfmt % tuple(current_values))

            # Write the whole chunk in one call, then flush to disk.
            write("\n".join(out))
            write("\n")
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
    TIME_FORMAT = "string"

    FILE_PATH = "./temp/very_large_dataset_5GB_2.csv"
    TARGET_SIZE_IN_GB = 5
    COLUMN_COUNT = 21
    COLUMN_SPACING = 100  # Set to 0 for all columns to start at the exact same value

    generate_large_csv(
        output_file=FILE_PATH,
        target_size_gb=TARGET_SIZE_IN_GB,
        total_cols=COLUMN_COUNT,
        timestamp_format=TIME_FORMAT,
        start_spacing=COLUMN_SPACING
    )
