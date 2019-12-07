import os

test_images = ['big_chessboard', 'big_quarter', 'chessboard', 'circle', \
	'diagonal_stripe', 'diffusion', 'ellipse_and_stripe', 'framed_white_noise',\
	'gradient', 'hash', 'letter_z', 'main_diagonal', 'parabole', 'quarter',\
	'single_point', 'solid_color', 'stripes', 'tiles', 'two_squares', 'white_noise']

test_folder = 'mytests_2'

for image in test_images:
	os.system('diff {tf}/{fn}_expected.bmp {tf}/{fn}_our.bmp'.format(tf=test_folder,\
							fn=image))

print('All tests are passed if no output is generated')
