import subprocess
import os

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

def read_filename_from_dir(dir):
    return os.listdir(dir)

if __name__ == "__main__":
    dir = "images"

    md5sum = Command('md5sum')

    jpeg_converter_src = Command('/root/C/AMX-Benchmarks/build_original_jpeg_source')
    jpeg_converter_dst = Command('/root/C/AMX-Benchmarks/build/libjpeg-benchmark/libjpeg-benchmark')

    invalid_image_set = []

    temp_dir = 'tmp'
    if not os.path.exists:
        os.mkdir(temp_dir)

    filenames = read_filename_from_dir(dir)
    print(filenames)

    for f in filenames:
        intput_filename = os.path.join(dir, f)

        src_md5 = os.path.join(temp_dir, f + '.src.md5')
        src_jpg = src_md5 + '.jpg'

        dst_md5 = os.path.join(temp_dir, f + '.dst.md5')
        dst_jpg = dst_md5 + '.jpg'

        try:
            jpeg_converter_src.append([
                intput_filename,
                src_md5
            ])
            jpeg_converter_dst.append([
                intput_filename,
                dst_md5
            ])
            jpeg_converter_src.execute()
            jpeg_converter_src.reset()

            jpeg_converter_dst.execute()
            jpeg_converter_dst.reset()

            md5sum.append([
                src_md5,
                dst_md5
            ])

            md5_text = md5sum.execute_then_output()
            md5sum.reset()

            md5set = []
            for l in md5_text.splitlines():
                md5set.append(l.split(' ', 2)[0])

            if md5set[0] != md5set[1]:
                print('md5 is not identical =>')
                for m in md5set:
                    print(m)

                raise Exception("Incorrect Md5")

            else:
                print("Test pass => ", intput_filename)
                print(md5_text)

        except:
            print("Failed on image file => ", intput_filename)
            invalid_image_set.append(intput_filename)
            with open("invalid_image_set.txt", "w", encoding="utf-8") as f:
                f.write(str(invalid_image_set))
                f.close()
            continue
