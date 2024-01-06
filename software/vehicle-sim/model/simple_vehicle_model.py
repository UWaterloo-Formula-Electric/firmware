"""
# High Level: Sum of X forces
## Ft = Torque force on rear tires
## Ff = friction force per tire
## Fd = drag force, negligible for now
ΣFx = m*a_x = 2*Ft - 4*Ff - Fd

# Ft
## d_t is diameter of tire
## T is torque
## 2 Axles assume distributed equally
Ft = T/(d_t/2)/2

# Ff
## μ is coefficient of friction of tire
## friction force distributed across each tire, assume weight fully balanced
Ff = μ*Fn

# Fd
## Take Fd = 0 for first iteration analysis
Fd = 0

# High Level: Sum of Y forces
## Take 0 y acceleration for first iteration analysis
## Fn normal force per tire
## Fg gravity force per tire
ΣFy = m*a_y = 4*Fn - 4*Fg

# Fn
## Dependent variable

# Fg
## m is mass of vehicle
## Take m/4 for mass per tire
Fg = (m/4) * g

# State Space
## State Variable x, x'
m*x'' = T/(d_t/2) - 4*μ*Fn - 0
m*0 = 4*Fn - m*g

x'' = T/(m*d_t/2) - 4*μ*Fn/m
x' = x'
Fn = m*g/4

[x''] = [0 0 -g*μ] [x'] + [1/(m*d_t/2)]
[x']  = [1 0 0] [x]  + [0] *T
[0]   = [0 0 0] [1]  + [0]

"""
from control import ss
import numpy as np

class SimpleVehicleModel():
    def get_system(p):
        A = np.matrix([[0 0 -4*p.tire_u*p.gravity],[1 0 0], [0 0 0]])
        B = np.matrix([1/(p.mass_kg)], [0], [0])
        C = np.matrix([[0 0 0],[0 0 0 ],[0 0 0]])
        D = np.matrix([[0],[0],[0]])
        return ss()
        

        
        
