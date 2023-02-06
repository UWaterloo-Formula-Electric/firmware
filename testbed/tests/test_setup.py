import slash
from testbeds.hil_testbed import hil_testbed
def test_setup(hil_testbed):
    pass

@slash.parametrize('x', [j for j in range(20)])
def test_math(x):
    sum = 0
    for j in range(x+1):
        sum = sum + j
    assert sum == x*(x+1)/2
