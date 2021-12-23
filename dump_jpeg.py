import os
import subprocess
import pandas as pd

if __name__ == '__main__':
    df = pd.DataFrame()

    dir = input("Enter the images source directory:")

    jpeg_list = os.listdir()