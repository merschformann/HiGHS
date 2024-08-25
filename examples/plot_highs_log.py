import re
import matplotlib.pyplot as plt
import numpy as np


def parse_highs_log(log_file_path):
    last_full_entry = []
    current_entry = []
    found_solution = False

    with open(log_file_path, "r") as f:
        for line in f:
            if "Running HiGHS" in line:
                if found_solution:
                    last_full_entry = current_entry
                current_entry = [line]
                found_solution = False
            else:
                current_entry.append(line)
                if "Writing the solution to" in line:
                    found_solution = True

    if not last_full_entry:
        last_full_entry = current_entry

    if not last_full_entry:
        return None, None, None, None, None, None

    time_values, best_bound_values, best_sol_values, in_queue_values, expl_values, gap_values = (
        [],
        [],
        [],
        [],
        [],
        [],
    )
    for line in last_full_entry:
        match = re.search(r"\dk?\s+\d+\.\ds$", line)

        if not match:
            continue

        tokens = line.split()
        if len(tokens) == 13:
            tokens = tokens[1:]
        assert len(tokens) == 12, f"{line}"

        in_queue_values.append(float(tokens[1]))  # InQueue
        expl_values.append(float(tokens[3].replace("%", "")))  # Expl.%
        best_bound_values.append(float(tokens[4].replace("inf", "nan")))  # Best Bound
        best_sol_values.append(float(tokens[5].replace("inf", "nan")))  # Best Sol
        gap_values.append(
            float(tokens[6].replace("%", "").replace("inf", "nan").replace("Large", "nan"))
        )  # Gap%
        time_values.append(float(tokens[11].replace("s", "")))  # Time

    return time_values, best_bound_values, best_sol_values, in_queue_values, expl_values, gap_values


def plot_highs_log(
    time_values, best_bound_values, best_sol_values, in_queue_values, expl_values, gap_values
):
    fig, ax1 = plt.subplots(figsize=(10, 6))

    # Plot Objective Bounds
    ax1.plot(time_values, best_bound_values, label="Best Bound", color="blue")
    ax1.plot(time_values, best_sol_values, label="Best Solution", color="green")
    ax1.set_xlabel("Time (seconds)")
    ax1.set_ylabel("Objective Bounds", color="blue")
    ax1.tick_params(axis="y", labelcolor="blue")

    # Limit y-axis to the range between min and max of the non-NaN values
    valid_gap_index = next(i for i, gap in enumerate(gap_values) if not np.isnan(gap))
    min_y = min(best_bound_values[valid_gap_index], best_sol_values[valid_gap_index])
    max_y = max(best_bound_values[valid_gap_index], best_sol_values[valid_gap_index])
    padding = (max_y - min_y) * 0.1
    ax1.set_ylim(min_y - padding, max_y + padding)

    # Add second y-axis for InQueue values
    ax2 = ax1.twinx()
    ax2.plot(time_values, in_queue_values, label="InQueue", color="red")
    #ax2.set_ylabel("InQueue", color="red")
    ax2.tick_params(axis="y", labelcolor="red")

    # Add third y-axis for Explored % values (scaled)
    ax3 = ax1.twinx()
    ax3.spines["right"].set_position(("outward", 30))
    ax3.plot(time_values, expl_values, label="Explored %", color="purple")
    #ax3.set_ylabel("Expl.%", color="purple")
    ax3.tick_params(axis="y", labelcolor="purple")

    # Add fourth y-axis for Gap % values (scaled)
    ax4 = ax1.twinx()
    ax4.spines["right"].set_position(("outward", 60))
    ax4.plot(time_values, gap_values, label="Gap %", color="orange")#, linestyle="--")#, linewidth=0.5)
    #ax4.set_ylabel("Relative gap.%", color="orange")
    ax4.tick_params(axis="y", labelcolor="orange")

    # Determine the interval for labeling Gap%
    #interval = max(1, len(time_values) // 10)  # Adjust interval for more labels

    # Add Gap% as text labels above the time axis, matching the color with Best Solution line
    #for i, (t, gap) in enumerate(zip(time_values, gap_values)):
    #if i % interval == 0:  # Label at defined intervals
    #ax1.text(
    #t,
    #ax1.get_ylim()[0],
    #f"{gap:.2f}%",
    #color="green",
    #ha="center",
    #va="bottom",
    #fontsize=10,
    #)

    # Add a Gap% label to indicate what the text values represent
    #ax1.text(
    #0.48,
    #0.05,
    #"Gap%",
    #color="green",
    #ha="left",
    #va="center",
    #transform=ax1.transAxes,
    #fontsize=8,
    #)

    # Plot vertical hash lines where Best Solution changes
    for i in range(1, len(best_sol_values)):
        if best_sol_values[i] != best_sol_values[i - 1]:  # Change detected
            ax1.axvline(x=time_values[i], color="grey", linestyle="--", linewidth=0.5)

    # Set up legend
    fig.legend(loc="lower left")

    # Show plot
    plt.title("HiGHS MIP Log Analysis")
    plt.show()


#log_file_path = "/path/to/your/logfile.log"
log_file_path = "HiGHS.log"
time_values, best_bound_values, best_sol_values, in_queue_values, expl_values, gap_values = (
    parse_highs_log(log_file_path)
)

plot_highs_log(
    time_values, best_bound_values, best_sol_values, in_queue_values, expl_values, gap_values
)
