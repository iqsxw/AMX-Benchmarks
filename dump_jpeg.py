import os
import subprocess
import pandas as pd
import threading

class Command():
    def __init__(self, exec) -> None:
        self.cmd = exec

    def append(self, options):
        if type(options) is type(list()):
            for o in options:
                self.append_with_space(o)
            return
        self.append_with_space(str(options))

    def append_with_space(self, s):
        self.cmd += ' ' + str(s)

    def execute(self):
        try:
            # print('execute => \n', self.cmd)
            subprocess.run(
                self.cmd,
                shell=True,
                check=True
                )
        except Exception:
            print("Failed to execute command. Check the output for details")
            raise

    def execute_then_output(self):
        # print('execute => \n', self.cmd)
        ret = ''
        try:
            ret = subprocess.check_output(
                self.cmd,
                shell=True,
                stderr=subprocess.STDOUT,
                ).decode("utf-8")
        except Exception:
            print("The command was incorrect. Check the output for details")
            raise
        return ret

    def reset(self):
        self.cmd = self.cmd.split(' ', 1)[0]

def dump_jpeg_subroutine(jpeg_list : list, dst):
    df = pd.DataFrame()
    for i in range(0, len(jpeg_list)):
        try:
            file = os.path.join(dir, jpeg_list[i])
            row = { 'Name': jpeg_list[i] }

            dump_jpeg.append([file])
            ret = dump_jpeg.execute_then_output()
            dump_jpeg.reset()

            props = ret.strip('\n').split(':')
            for p in props:
                seq = p.split('=')
                row[seq[0]] = seq[1]
            df = df.append(row, ignore_index=True)
            print("process " + str(i) + '/' + str(len(jpeg_list)))
        except:
            print("Failed on ", file)
            continue

    df.to_csv(dst, encoding='utf-8')
    print('"Write to "' + dst + '" With Success')

if __name__ == '__main__':
    df = pd.DataFrame()

    dir = input("Enter the images source directory:")

    dump_jpeg = Command('./dump_jpeg')

    tmp_dir = 'csv'
    if not os.path.exists(tmp_dir):
        os.mkdir(tmp_dir)

    jpeg_list = os.listdir(dir)

    threads = []
    dst = os.path.join(tmp_dir, dir.split('/')[-1] + '.csv')
    for i in range(0, 1):
        n = int(len(jpeg_list) / 1)
        x = threading.Thread(target=dump_jpeg_subroutine, args=(jpeg_list[i * n : i * n + n], dst))
        threads.append(x)
        x.start()

    for i, thread in enumerate(threads):
        thread.join()
        print('Thread: ' + str(i) + ' Finished')
