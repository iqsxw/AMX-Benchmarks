import numpy as np

def fn(x, s):
    return ((x) + ((((x) - (1 << ((s) - 1))) >> 31) & (((-1) << (s)) + 1)))

a = np.zeros(2048 * 12, dtype=int).reshape(12, 2048)

for i in range(1, 12):
    k = i
    m = 0
    for j in range(0, pow(2, i)):
            a[k][m] = fn(j, i)
            m = m + 1

for i in a:
    s = ''
    for j in i:
        s += str(j) + ', '
    print(s)
