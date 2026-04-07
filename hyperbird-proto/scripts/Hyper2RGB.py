from scipy.interpolate import PchipInterpolator
from bisect import bisect
import numpy as np

__author__ = "Dani Martinez"
__copyright__ = "Copyright 2023, Moblanc Robotics"
__credits__ = ["Dani Martinez"]
__license__ = "GPL"
__version__ = "1.0.0"
__maintainer__ = "Dani Martinez"
__email__ = "dmartinezla1@gmail.com"

""" Hyper-spectral image to RGB image conversion based on:

M. Magnusson, J. Sigurdsson, S. E. Armansson, M. O. Ulfarsson, H. Deborah and J. R. Sveinsson
"Creating RGB Images from Hyperspectral Images Using a Color Matching Function,"
IGARSS 2020 - 2020 IEEE International Geoscience and Remote Sensing Symposium, 2020, pp. 2045-2048, doi: 10.1109/IGARSS39084.2020.9323397.
"""

class Hyper2RGB:
    def __init__(self, illuminant=65):
        self.illuminant = illuminant
        # Hyper image to RGB required data
        self.wl = []
        self.x = []
        self.y = []
        self.z = []
        self.I = None
        self.wl_imax = 0

        self.vbin = -1

    def init(self,vbin):
        i = {50:2, 55:3, 65:1, 75:4} # Illumiant : index

        if self.vbin == vbin:
            return

        d_data = np.genfromtxt('data/D.csv', delimiter=',')
        wxyz_data = np.genfromtxt('data/wxyz.csv', delimiter=',')
        self.wl = np.squeeze(np.genfromtxt('data/spectral_axis_cropped.txt')).tolist()
        self.wl = [self.wl[i] for i in range(0,len(self.wl),vbin)]

        self.vbin = vbin

        w = wxyz_data[:,0]
        x = wxyz_data[:,1]
        y = wxyz_data[:,2]
        z = wxyz_data[:,3]

        wI = d_data[:,0]
        I = d_data[:,i[self.illuminant]]

        # Interpolate to image wavelengths
        I = PchipInterpolator(wI,I,extrapolate=True)(self.wl) # interp1(wI,I,wY,'pchip','extrap')';
        x = PchipInterpolator(w,x,extrapolate=True)(self.wl) # interp1(w,x,wY,'pchip','extrap')';
        y = PchipInterpolator(w,y,extrapolate=True)(self.wl) # interp1(w,y,wY,'pchip','extrap')';
        z = PchipInterpolator(w,z,extrapolate=True)(self.wl) # interp1(w,z,wY,'pchip','extrap')';

        #plt.plot(wl,y)
        #plt.show()

        # Truncate at 780nm
        self.wl_imax=bisect(self.wl, 780)
        self.wl=self.wl[:self.wl_imax]
        self.I=I[:self.wl_imax]
        self.x=x[:self.wl_imax]
        self.y=y[:self.wl_imax]
        self.z=z[:self.wl_imax]

    def hyperimg_to_rgb(self, hyperimg ,threshold):
        #import cv2
        # Normalize to 255
        #hyperimg = ((hyperimg*255)/4095).astype(np.uint8)

        (ydim, xdim, zdim) = hyperimg.shape

        # Reorder data so that each column holds the spectra of of one pixel
        HSI_data = np.reshape(hyperimg, [-1,zdim])/hyperimg.max()
        HSI_data = HSI_data[:,0:self.wl_imax]/HSI_data.max()

        # Compute k
        k = 1/np.trapz(self.y * self.I, self.wl)
        
        # Compute X,Y & Z for image
        X = k * np.trapz(HSI_data @ np.diag(self.I * self.x), self.wl, axis=1)
        Z = k * np.trapz(HSI_data @ np.diag(self.I * self.z), self.wl, axis=1)
        Y = k * np.trapz(HSI_data @ np.diag(self.I * self.y), self.wl, axis=1)
        
        XYZ = np.array([X, Y, Z])
        
        # Convert to RGB
        M = np.array([[3.2404542, -1.5371385, -0.4985314],
                    [-0.9692660, 1.8760108, 0.0415560],
                    [0.0556434, -0.2040259, 1.0572252]])
        sRGB=M@XYZ

        # Gamma correction
        gamma_map = sRGB >  0.0031308
        sRGB[gamma_map] = 1.055 * np.power(sRGB[gamma_map], (1. / 2.4)) - 0.055
        sRGB[np.invert(gamma_map)] = 12.92 * sRGB[np.invert(gamma_map)]
        
        # Note: RL, GL or BL values less than 0 or greater than 1 are clipped to 0 and 1.
        sRGB[sRGB > 1] = 1
        sRGB[sRGB < 0] = 0
        
        if threshold:
            for idx in range(3):
                y = sRGB[idx,:]
                a,b = np.histogram(y,100)
                b = b[:-1] + np.diff(b)/2
                a=np.cumsum(a)/np.sum(a)
                th = b[0]
                i = a<threshold
                if i.any():
                    th=b[i][-1]
                y=y-th
                y[y<0] = 0

                a,b=np.histogram(y,100)
                b = b[:-1] + np.diff(b)/2
                a=np.cumsum(a)/np.sum(a)
                i = a > 1-threshold
                th=b[i][0]
                y[y>th]=th
                y=y/th
                sRGB[idx,:]=y
            
        R = np.reshape(sRGB[0,:],[ydim,xdim])
        G = np.reshape(sRGB[1,:],[ydim,xdim])
        B = np.reshape(sRGB[2,:],[ydim,xdim])
        return np.transpose(np.array([R,G,B]),[1,2,0])
    
    @staticmethod
    def gammaCorrection(src, gamma):
        import cv2
        invGamma = 1 / gamma

        table = [((i / 255) ** invGamma) * 255 for i in range(256)]
        table = np.array(table, np.uint8)

        return cv2.LUT(src, table)

    @staticmethod
    def WB_mean(img):
        return (img*1.0 / img.mean(axis=(0,1)))

    @staticmethod
    def WB_max(img):
        return (img*1.0 / img.max(axis=(0,1)))

    # Use around 0.9
    @staticmethod
    def WB_percentile(img, percentile_value):
        img = ((img*1.0 / np.percentile(img, percentile_value, axis=(0, 1))).clip(0, 1))
        img = 255 * img # Now scale by 255
        return img.astype(np.uint8)
    
    @staticmethod
    def WB_greyworld(img):
        return ((img * (img.mean() / img.mean(axis=(0, 1)))).clip(0, 255).astype(int))