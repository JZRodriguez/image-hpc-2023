# image-hpc-2023

The main implementation done is the modification of the image. The changes done and sending the result back to reassemlbe them again.

Usage:
```
mpicc Img.c -lm -lspng -o ./Img
mpiexec -l -n 1 ./Img
```
 
Expected output:
```
[0] width: 844
[0] height: 655
[0] bit depth: 8
[0] color type: 6 - truecolor with alpha
[0] compression method: 0
[0] filter method: 0
[0] interlace method: 0
[0] palette entries: 0
```

Screenshot:
![Image](Results.png)
