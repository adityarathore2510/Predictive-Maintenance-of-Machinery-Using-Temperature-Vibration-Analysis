# 🔧 Predictive Maintenance of Machinery Using Temperature & Vibration Analysis

<div align="center">

![Python](https://img.shields.io/badge/Python-3.10-blue?logo=python)
![Machine Learning](https://img.shields.io/badge/Machine_Learning-Predictive_Analytics-green)
![Pandas](https://img.shields.io/badge/Pandas-Data_Analysis-orange?logo=pandas)
![Status](https://img.shields.io/badge/Status-Completed-brightgreen)
![Industry 4.0](https://img.shields.io/badge/Industry-4.0-blueviolet)

### Intelligent Fault Detection and Predictive Maintenance System

</div>

---

# 📖 Overview

Unexpected machinery failures can result in costly downtime, reduced productivity, and expensive repairs. This project develops a Predictive Maintenance System that analyzes temperature and vibration data to identify abnormal machine behavior before a breakdown occurs.

The system demonstrates how data-driven monitoring can improve equipment reliability and reduce maintenance costs.

---

# 🎯 Project Objectives

✅ Monitor machine health using sensor data

✅ Detect abnormal operating conditions

✅ Identify potential equipment failures

✅ Visualize machinery performance trends

✅ Demonstrate predictive maintenance concepts

---

# 🧠 Working Principle

Healthy machines operate within normal temperature and vibration ranges.

When components begin to wear out:

* Temperature increases
* Vibration levels rise
* Operating patterns become abnormal

By continuously monitoring these parameters, the system can identify deviations from normal behavior and generate maintenance alerts before catastrophic failure occurs.

This approach is widely used in:

* Smart Factories
* Industry 4.0 Systems
* Manufacturing Plants
* Power Generation Facilities

---

# ⚙️ System Architecture

```text
Temperature Sensor Data
          │
          ▼
Data Collection
          │
          ▼
Data Cleaning & Processing
          │
          ▼
Feature Extraction
          │
          ▼
Anomaly Detection
          │
          ▼
Health Assessment
          │
          ▼
Maintenance Alert
```

---

# 🛠️ Technologies Used

| Technology   | Purpose               |
| ------------ | --------------------- |
| Python       | Programming           |
| Pandas       | Data Processing       |
| NumPy        | Numerical Computation |
| Matplotlib   | Data Visualization    |
| Scikit-Learn | Predictive Analysis   |
| GitHub       | Version Control       |

---

# 📂 Repository Structure

```text
Predictive-Maintenance/
│
├── dataset/
│   ├── machine_data.csv
│
├── src/
│   ├── data_preprocessing.py
│   ├── anomaly_detection.py
│   ├── visualization.py
│
├── results/
│   ├── temperature_trends.png
│   ├── vibration_analysis.png
│
├── report/
│   └── Project_Report.pdf
│
└── README.md
```

---

# 🔬 Methodology

### Step 1: Data Collection

Collect temperature and vibration measurements from machinery sensors.

### Step 2: Data Preprocessing

Remove missing values and normalize sensor readings.

### Step 3: Feature Analysis

Analyze patterns and trends in machine behavior.

### Step 4: Fault Detection

Identify abnormal conditions using threshold-based or machine-learning methods.

### Step 5: Maintenance Prediction

Generate alerts when indicators exceed acceptable limits.

---

# 📊 Sample Dataset

| Time  | Temperature (°C) | Vibration (mm/s) |
| ----- | ---------------- | ---------------- |
| 00:00 | 38               | 1.2              |
| 01:00 | 39               | 1.3              |
| 02:00 | 40               | 1.4              |
| 03:00 | 47               | 2.8              |
| 04:00 | 54               | 4.1              |

---

# 📈 Results & Analysis

### Temperature Trend

* Stable operation observed below 45°C.
* Rapid temperature increase indicates possible component stress.

### Vibration Trend

* Normal vibration range maintained during healthy operation.
* Significant vibration spikes detected before fault conditions.

### Maintenance Insights

✔ Early fault identification

✔ Reduced unplanned downtime

✔ Improved equipment reliability

✔ Enhanced operational efficiency

---

# 📸 Output Visualizations

## Temperature Monitoring

```text
Temperature (°C)

60 ┤
55 ┤                ●
50 ┤
45 ┤          ●
40 ┤     ●
35 ┤ ●
   └───────────────────► Time
```

*(Replace with actual graph generated from Python)*

---

## Vibration Analysis

```text
Vibration

5 ┤                ●
4 ┤
3 ┤          ●
2 ┤
1 ┤ ● ● ●
  └────────────────► Time
```

*(Replace with actual matplotlib output)*

---

# 🏭 Applications

* Industrial Automation
* Manufacturing Plants
* Smart Factories
* Power Plants
* Oil & Gas Industry
* Heavy Machinery Monitoring

---

# 🚀 Future Improvements

* Real-Time IoT Sensor Integration
* Cloud-Based Monitoring Dashboard
* Deep Learning Fault Detection
* Edge Computing Deployment
* Mobile Alert System
* Predictive Remaining Useful Life (RUL) Estimation

---

# 📚 References

1. Industry 4.0 Predictive Maintenance Frameworks
2. IEEE Research Papers on Condition Monitoring
3. Scikit-Learn Documentation
4. Predictive Maintenance Engineering Handbook

---

# 👨‍💻 Author

**Aditya Rathore**

Electronics & Communication Engineering


---

⭐ If you found this project useful, consider starring the repository.
