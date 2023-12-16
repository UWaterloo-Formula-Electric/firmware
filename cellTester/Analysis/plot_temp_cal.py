import plotly.express as px
import pandas as pd

# Read in data
df = pd.read_csv("Dec-15_20-50-39temp_cal.csv")
df["Voltage1 [mV]"] = df["Voltage1 [mV]"] / 1000
# Plot data
fig = px.scatter(df, x="Timestamp [ms]", y=["True Temp [C]","Temp1 [C]", "Voltage1 [mV]"])
fig.show()
