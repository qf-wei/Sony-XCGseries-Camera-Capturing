import subprocess
import os
import time
import numpy as np
import cv2
import mmap

class CameraController:
    def __init__(self, cameraNum):
        executable_path = f'lib/CAM{cameraNum}.exe'
        ## Input the path to the camera executable here
        self.SHM_NAME = f"Local\\Cam{cameraNum}Mem"
        ## Input the shared memory name here
        ## The shared memory name should be the same as the one in the camera executable

        self.SHM_SIZE = 1024 * 768 * 4
        ## Input image size here with RGBA

        self.CamEnv = os.environ.copy()
        self.shared_memory = mmap.mmap(-1, self.SHM_SIZE, self.SHM_NAME)
        self.camera_app = subprocess.Popen(
            [executable_path],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            bufsize=0,
            env=self.CamEnv,
            text=False
        )

        print(f"Camera{cameraNum} Initializing...")
        time.sleep(5)
        self.read_camera_model()

    def read_camera_model(self):
        line = self.camera_app.stdout.readline()
        self.CamSerial = line.decode().strip()
        return line.decode().strip()

    def capture_image(self):
        self.camera_app.stdin.write(b'capture\n')
        # Sending bytes to capture the image
        self.camera_app.stdin.flush()
        self.camera_app.stdout.readline()
        # Waiting for the camera to finish capturing
        image_data = self.shared_memory.read(self.SHM_SIZE)
        # Reading the image from the shared memory
        self.shared_memory.seek(0)
        image_array = np.frombuffer(image_data, np.uint8)
        return self.imagePreProcess(image_array)
    
    def finalize_camera(self):
        self.camera_app.stdin.write(b'finalize\n')  
        # Sending bytes to shotdown the camera
        self.camera_app.stdin.flush()
        self.camera_app.terminate()
        self.camera_app.wait()
        self.shared_memory.close()

    def imagePreProcess(self, image_array):
        image_array = image_array.reshape(768,1024,4)
        return np.flip(image_array, axis=0)[:,:,0:3]
