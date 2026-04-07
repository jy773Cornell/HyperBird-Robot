import argparse
from matplotlib import pyplot as plt
import numpy as np
import os

__author__ = "Dani Martinez"
__copyright__ = "Copyright 2023, Moblanc Robotics"
__credits__ = ["Dani Martinez"]
__license__ = "GPL"
__version__ = "1.0.0"
__maintainer__ = "Dani Martinez"
__email__ = "dmartinezla1@gmail.com"

herror = '\033[91m[ERROR]\033[0m: '

MAX_UINT16 = 65535
MAX_UINT12 = 4095

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('traydir',metavar='<TRAY_DIR>', help="Path to a tray folder",type=str)
    args = parser.parse_args()

    im_tray_path = os.path.abspath(args.traydir)
    if not os.path.exists(im_tray_path):
        print(herror+'Input folder path does not exist!')
        exit()


    hyperimg_files = []
    for file in os.listdir(im_tray_path):
        # check only text files
        if file.endswith('.raw'):
            hyperimg_files.append(file)

    if not hyperimg_files:
        print(herror+' No hyper images found in provided TRAY directory path!')
        exit()


    for img in hyperimg_files:

        im_file_path = im_tray_path+'/'+img
        info_file_path = im_file_path[:-3]+'hdr'

        if not os.path.exists(info_file_path):
            print(herror+'Input info txt file does not exist!')
            exit()

        n_frames = 0
        im_width = 0
        im_height = 0
        bitspix = 12

        # Read Hyper-Image info
        with open(info_file_path) as file:
            lines = [line.rstrip().split('=') for line in file]
        for l in lines:
            attr = l[0].strip()
            if attr == 'lines':
                n_frames = int(l[1])
            elif attr == 'samples':
                im_width = int(l[1])
            elif attr == 'bands':
                im_height = int(l[1])
            elif attr == 'gain mode':
                bitspix = int(l[1][1:3])

        # Load RAW image data from file, reshape to proper dimensions
        hyper_img = np.fromfile(im_file_path,dtype=np.uint16).reshape((n_frames,im_width,im_height))

        #print(hyper_img.shape)

        # Compute 2D image from maximum intensity in all bands per row
        max_image = np.amax(hyper_img,axis=2)

        if bitspix == 12:
            max_value = MAX_UINT12
        else:
            max_value = MAX_UINT16

        # Normalize to UINT8
        max_image = ((max_image.astype(np.uint32)*255)/max_value).astype(np.uint8)

        plt.imshow(max_image)
        plt.grid(False)
        plt.xticks([])
        plt.yticks([])
        print('Generating '+im_file_path+' preview image...')
        plt.savefig( im_file_path[:-3] + 'png', bbox_inches='tight', transparent="True", pad_inches=0)

