import argparse
from matplotlib import pyplot as plt
import numpy as np
import os
import cv2
from Hyper2RGB import Hyper2RGB
from PIL import Image, ImageCms

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
    parser.add_argument('hyperimg',metavar='<HYPER_IMG_RAW>', help="Input Hyper-image file (.raw)",type=str)
    args = parser.parse_args()

    im_file_path = os.path.abspath(args.hyperimg)
    info_file_path = im_file_path[:-3]+'hdr'

    if not os.path.exists(im_file_path):
        print(herror+'Input raw file does not exist!')
        exit()

    if not os.path.exists(info_file_path):
        print(herror+'Input info txt file does not exist!')
        exit()

    RGBconv = Hyper2RGB()

    n_frames = 0
    im_width = 0
    im_height = 0
    spectral_bin = 1
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
        elif attr == 'vbin':
            spectral_bin = int(l[1][2:])
        elif attr == 'gain mode':
            bitspix = int(l[1][1:3])

    # Load RAW image data from file, reshape to proper dimensions
    #hyper_img = np.fromfile(im_file_path,dtype=np.int32).reshape((n_frames,im_width,im_height))

    hyper_img = np.fromfile(im_file_path,dtype=np.uint16).reshape((n_frames,im_width,im_height))

    RGBconv.init(spectral_bin)
    rgb_image = RGBconv.hyperimg_to_rgb(hyper_img,0.0)


    # Convert from  
    #rgb_image = cv2.normalize(rgb_image, None, 255, 0, cv2.NORM_MINMAX, cv2.CV_8U)
    #rgb_image = cv2.cvtColor(rgb_image, cv2.COLOR_RGB2BGR)
    
    #rgb_image = gammaCorrection(rgb_image, 2.2)

    #cv2.imshow('Color image', rgb_image)
    #cv2.waitKey(0)
    #exit()

    #rgb_image = RGBconv.WB_max(rgb_image)
    #rgb_image = RGBconv.WB_percentile(rgb_image, 0.9)
    #rgb_image = RGBconv.WB_greyworld(rgb_image)


    rgb_image = 255 * rgb_image # Now scale by 255
    rgb_image = rgb_image.astype(np.uint8)

    # WHITE BALANCE NEED TO BE APPLIED
    rgb_image = cv2.cvtColor(rgb_image, cv2.COLOR_RGB2BGR)
    cv2.imwrite(im_file_path[:-4] + '_rgb.png',rgb_image)

    """
    plt.figure(figsize=(10,6))
    plt.imshow(rgb_image)
    plt.grid(False)
    plt.xticks([])
    plt.yticks([])
    #plt.show()
    plt.savefig( im_file_path[:-4] + '_rgb.png', bbox_inches='tight', transparent="True", pad_inches=0)
    """
    
    # Compute 2D image from maximum intensity in all bands per row
    max_image = np.amax(hyper_img,axis=2)

    if bitspix == 12:
        max_value = MAX_UINT12
    elif bitspix == 16:
        max_value = MAX_UINT16

    # Normalize to UINT8. First is converted to uint32 to avoid overflow
    max_image = ((max_image.astype(np.uint32)*255)/max_value).astype(np.uint8)

    plt.figure(dpi=600)
    plt.imshow(max_image)
    plt.savefig( im_file_path[:-4] + '_max.png', bbox_inches='tight', transparent="True", pad_inches=0,dpi=600)

