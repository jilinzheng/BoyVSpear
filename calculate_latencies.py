import datetime
import pytz
import pandas as pd


ESP32_FILE = "esp32_cmd_sent_times.txt"
RPI3_FILE = "rpi3_cmd_receive_times.txt"
# data recorded May 4, 2025
log_date = datetime.date(2025, 5, 4)
timezone_edt = pytz.timezone("America/New_York")


# convert 'HH:MM:SS.ms' timestamp (EDT) for a specific date to UTC Unix timestamp
def convert_arduino_timestamp_edt_to_utc_unix(time_str, date, timezone):
    try:
        # Parse the time string
        time_obj = datetime.datetime.strptime(time_str, "%H:%M:%S.%f").time()
        # Combine with the date and localize to the specified timezone
        dt_aware = timezone.localize(datetime.datetime.combine(date, time_obj))
        # Convert to UTC and then to Unix timestamp
        return dt_aware.astimezone(pytz.utc).timestamp()
    except ValueError as e:
        print(f"Error parsing timestamp {time_str}: {e}")
        return None


# read RPi timestamps
rpi_timestamps = []
try:
    with open(RPI3_FILE, "r") as f:
        for line in f:
            try:
                rpi_timestamps.append(float(line.strip()))
            except ValueError as e:
                print(f"Error converting RPi timestamp to float: {line.strip()} - {e}")
except FileNotFoundError:
    print("Error: rpi3_cmd_receive_times.txt not found.")
    exit()


# process ESP32 timestamps
esp_timestamps_edt_str = []
esp_timestamps_utc_unix = []
try:
    with open(ESP32_FILE, "r") as f:
        for line in f:
            parts = line.strip().split(" -> ")
            if len(parts) == 2:
                time_str = parts[0]
                esp_timestamps_edt_str.append(time_str)
                utc_unix_ts = convert_arduino_timestamp_edt_to_utc_unix(
                    time_str, log_date, timezone_edt
                )
                esp_timestamps_utc_unix.append(utc_unix_ts)
            else:
                print(
                    f"Skipping malformed line in esp32_cmd_sent_times.txt: {line.strip()}"
                )
except FileNotFoundError:
    print("Error: esp32_cmd_sent_times.txt not found.")
    exit()


# ensure both lists have the same number of valid timestamps before creating DataFrame
# filter out any None values that resulted from parsing errors
valid_esp_timestamps = [
    (edt_str, unix_ts)
    for edt_str, unix_ts in zip(esp_timestamps_edt_str, esp_timestamps_utc_unix)
    if unix_ts is not None
]
esp_timestamps_edt_str_filtered = [item[0] for item in valid_esp_timestamps]
esp_timestamps_utc_unix_filtered = [item[1] for item in valid_esp_timestamps]

# truncate the longer list to match the length of the shorter list if they differ
min_len = min(len(esp_timestamps_utc_unix_filtered), len(rpi_timestamps))
esp_timestamps_edt_str_final = esp_timestamps_edt_str_filtered[:min_len]
esp_timestamps_utc_unix_final = esp_timestamps_utc_unix_filtered[:min_len]
rpi_timestamps_final = rpi_timestamps[:min_len]

# create a DataFrame
if min_len > 0:
    df_latency = pd.DataFrame(
        {
            "timestamp_esp_edt": esp_timestamps_edt_str_final,
            "timestamp_esp_utc_unix": esp_timestamps_utc_unix_final,
            "timestamp_rpi_utc_unix": rpi_timestamps_final,
        }
    )

    # calculate the latency
    df_latency["latency_seconds"] = (
        df_latency["timestamp_rpi_utc_unix"] - df_latency["timestamp_esp_utc_unix"]
    )

    # display the results
    print("\nCalculated Latencies (seconds):")
    print(
        df_latency[["timestamp_esp_edt", "timestamp_rpi_utc_unix", "latency_seconds"]]
    )

    # calculate and display mean and median latency
    mean_latency = df_latency["latency_seconds"].mean()
    median_latency = df_latency["latency_seconds"].median()

    print(
        f"\nMean Latency: {mean_latency:.6f} seconds = {mean_latency*1000:.6f} milliseconds"
    )
    print(
        f"Median Latency: {median_latency:.6f} seconds = {median_latency*1000:.6f} milliseconds"
    )
else:
    print(
        "\nNo valid corresponding timestamps found in both files to calculate latency."
    )
