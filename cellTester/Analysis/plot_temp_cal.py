import plotly.express as px
import plotly.graph_objects as go
import pandas as pd
import numpy as np
from scipy.interpolate import splrep, splev, make_smoothing_spline, PPoly
from scipy.optimize import curve_fit
from lmfit.models import StepModel, SplineModel
from sympy import lambdify, bspline_basis_set
from sympy.abc import u

df = pd.read_csv("Dec-15_20-50-39temp_cal.csv")


def wheatstone_V_to_R(V):
    VOLTAGE_IN = 3.3
    R1 = 10000
    R = R1 * (VOLTAGE_IN - 2*V)/(VOLTAGE_IN + 2*V)
    return R


def temp_steinhart_hart(R):
    A = 0.00112864
    B = 0.000233122
    C = 8.3895e-8
    ln_R = np.log(R)
    T = 1/(A + B * ln_R + C * ln_R**3) - 273.15
    return T


def find_stats(df: pd.DataFrame, period=None):
    mean = np.mean(df)
    std = np.std(df)
    max_jitter = np.max(df) - mean
    min_jitter = np.min(df) - mean
    if abs(max_jitter) > abs(min_jitter):
        max_deviation = max_jitter
    else:
        max_deviation = min_jitter
    return mean, std, max_deviation


# function to set background color for a
# specified variable and a specified level
def highLights(fig, df, condition, fillcolor, layer):
    df1 = df[condition].groupby((~condition).cumsum())[
        'Timestamp [ms]'].agg(['first', 'last'])

    for _, row in df1.iterrows():
        fig.add_shape(type="rect", xref="x", yref="paper", x0=row['first'], y0=0, x1=row['last'], y1=1,
                      line=dict(color="rgba(0,0,0,0)", width=3,), fillcolor=fillcolor, layer=layer)
    return fig


oneLSB = 3.3 / 2**15
df.drop(df[(df["True Temp [C]"] > 103) | (
    df["True Temp [C]"] < 30)].index, inplace=True)
df["Voltage1"] = df["Voltage1 [mV]"] / 1000
df["Timestamp [s]"] = df["Timestamp [ms]"] / 1000
df["R [Ohm]"] = wheatstone_V_to_R(df["Voltage1"])
df["Model Temp [C]"] = temp_steinhart_hart(df["R [Ohm]"])

# error between true temp and computed temp
df["Error [C]"] = df["True Temp [C]"] - df["Model Temp [C]"]


t = splrep(df["Timestamp [s]"], df["Temp1 [C]"], k=5, s=0.6)
tt = splrep(df["Timestamp [s]"], df["True Temp [C]"], k=5, s=1)


df["Fitted Temp1"] = splev(df["Timestamp [s]"], t)
df["dTemp1/dt"] = splev(df["Timestamp [s]"], t, der=1)

df["Fitted True Temp"] = splev(df["Timestamp [s]"], tt)
df["dTrue Temp/dt"] = splev(df["Timestamp [s]"], tt, der=1)

spl = make_smoothing_spline(df["Timestamp [s]"], df["Voltage1"], lam=3)
df["Fitted Voltage1"] = spl(df["Timestamp [s]"])
df["dV1/dt"] = spl.derivative()(df["Timestamp [s]"])

print(find_stats(df["Fitted Voltage1"] - df["Voltage1"]))
print(find_stats(df["Fitted Temp1"] - df["Temp1 [C]"]))

threshold = 0.02
condition = (abs(df["dTemp1/dt"]) <= threshold) | (df["True Temp [C]"] <= 35)
# Plot data
#  Uncomment to plot raw data with flat regions highlighted
# fig = px.scatter(df, x="Timestamp [ms]", y=["True Temp [C]", "Voltage1", "Temp1 [C]", "Fitted Temp1", "Fitted Voltage1", "dV1/dt", "dTemp1/dt"],
#                  title="Temperature Calibration")
# fig = highLights(fig, df, condition, 'rgba(200,0,200,0.1)', "below")
# fig.show()

df.drop(df[~condition].index, inplace=True)

# calculate polynomial
x = df["Voltage1"].to_numpy()
y = df["True Temp [C]"].to_numpy()


def curve(x, a, b, c):
    """Define what type of curve we want to fit, params are the coefficients"""
    R = wheatstone_V_to_R(x)
    ln_R = np.log(R)
    T = 1/(a + b * ln_R + c * ln_R**3) - 273.15
    return T


# taken from existing values on cell tester
initial_guess = [0.00112864, 0.000233122, 8.3895e-8]
coeffs, pcov = curve_fit(curve, x, y, p0=initial_guess, method="lm")

print(f"{coeffs=}")
y_new = curve(x, *coeffs)
residuals = y_new - y
r_squared = 1 - np.var(residuals) / np.var(y)

print(f"{r_squared=}")
print("stats =",find_stats(y - y_new))

x_new = np.linspace(x[0], x[-1], 5000)
y_new = curve(x_new, *coeffs)

data = [
    go.Scatter(x=df["Voltage1"], y=df["True Temp [C]"],
               mode='markers', name="True Temp [C]", marker={"size": 5.5}),
    go.Scatter(x=x_new, y=y_new, mode='lines',
               name="Modelled Temp [C]", marker={"size": 5.5}),
    go.Scatter(x=df["Voltage1"], y=df["Temp1 [C]"],
               mode='markers', name="Temp1 [C]", marker={"size": 5.5}),
]

v_to_r_eqn = r"$R_{therm}=10000\frac{(3.3-2V)}{(3.3+2V)}$"
equation = f"$({coeffs[0]:.6} + {coeffs[1]:.6}" "*ln(R_{therm}) + " f"{coeffs[2]:.6}" "*ln(R_{therm})^3)^{-1} - 273.15$"
annotations = [
    go.layout.Annotation(
        x=0.9, y=40, text=r"$R^2 = " f"{r_squared:.6}$", showarrow=False, font={"size": 20}),
    go.layout.Annotation(x=0.9, y=90, text=v_to_r_eqn,
                         showarrow=False, font={"size": 40}),
    go.layout.Annotation(x=0.9, y=80, text=equation,
                         showarrow=False, font={"size": 20}),
]

layout = go.Layout(title="Temperature Calibration",
                   xaxis_title="Voltage1 [V]", yaxis_title="Temperature [C]",
                   annotations=annotations)
fig = go.Figure(data=data, layout=layout)
fig.show()
