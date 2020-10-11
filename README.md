<div align="left">
  <h1>Image Processing</h1>
  <h3>A C program that edits images in a few cool ways</h3>
</div>
<br/>
<br/>

## How to use ##
Firstly, edit the input.txt and make sure to use your desired image and editing parameters.

The rows in input.txt correspond to:
* relative path to the image to be edited
* relative path to a file that contains the filter you want to use
* relative path to a file that contains the pooling parameters you want to use
* relative path to a file that contains the clusering parameters you want to use

The text file with the filter must contain the dimension of the filter matrix on the first row,
and the filter matrix on the following rows.

Example:

`3
1 -1 0
0 1 0
0 0 1`

The text file with the pooling parameters must contain either 'M' or 'm' for Max or min pooling layer filters
and the size of the filter (on the same row). 

Example:

`M 5`

The text file with the clustering parameters you want to use must contain a positive integer that stands for
the clustering threshold.

To run the program, use either the Makefile, your IDE or terminal.

A few sample images and filters/parameters files have been included in the input folder.
<br><br><br>

## Output ##
The program computes and outputs:
* The black & white version of the specified image
* The No-Crop version of the specified image (makes a rectangular image square-shaped
by adding a white border)
* The filtered image
* The Max/Min Pooling Layer filters applied to the image
* The image that results after clustering
