# SPDX-License-Identifier: MIT

import hashlib
import os

import cv2
# import imgui
import pygame as g
import pygame.locals as gc
import tkinter
from tkinter import filedialog

if __name__ == '__main__':
    # Ask for the folder containing the source images
    tkinter.Tk().withdraw()
    source_dir = filedialog.askdirectory(title='Select the source folder')

    # Ask for output folder
    output_dir = filedialog.askdirectory(title='Select the output folder')

    # Recursively search for images
    sources = []
    for root, _, filenames in os.walk(source_dir):
        for filename in filenames:
            if filename.endswith('.jpg') or filename.endswith('.png'):
                sources.append(os.path.join(root, filename))
    # sources = sources[:50]

    g.init()

    print('Press space to go to the next image')
    # print('Press escape to quit')
    print('Press R to undo')
    print('Press Q to restart the image')

    # Walk image list and extract faces
    early_quit = False
    for i, source_path in enumerate(sources):
        if early_quit:
            break

        source = cv2.imread(source_path)

        # Set up window
        surf = g.display.set_mode((source.shape[1], source.shape[0]))
        g.display.set_caption(f'[{i+1}/{len(sources)}] {source_path}')

        crop_set = []

        image_done = False
        while not image_done:
            # Handle events
            for evt in g.event.get():
                if evt.type == gc.KEYDOWN:
                    if evt.key == gc.K_SPACE:
                        image_done = True
                    # elif evt.key == gc.K_ESCAPE:
                    #     image_done = True
                    #     early_quit = True
                    elif evt.key == gc.K_r:
                        crop_set.pop()
                    elif evt.key == gc.K_q:
                        crop_set = []
                elif evt.type == gc.MOUSEBUTTONDOWN:
                    crop_set.append([evt.pos[0], evt.pos[1], evt.pos[0], evt.pos[1]])
                elif evt.type == gc.QUIT:
                    image_done = True
                    early_quit = True

            mouse_pos = g.mouse.get_pos()
            if g.mouse.get_pressed()[0]:
                crop_set[-1][2] = mouse_pos[0]
                crop_set[-1][3] = mouse_pos[1]
                w = crop_set[-1][2] - crop_set[-1][0]
                h = crop_set[-1][3] - crop_set[-1][1]
                sz = max(w, h)
                crop_set[-1][2] = crop_set[-1][0] + sz
                crop_set[-1][3] = crop_set[-1][1] + sz

            # Update screen
            panel = g.image.frombuffer(source, source.shape[1::-1], 'BGR')
            surf.blit(panel, (0, 0))
            if not g.mouse.get_pressed()[0]:
                g.draw.line(surf, (255, 0, 0), g.Vector2(0, mouse_pos[1]), g.Vector2(source.shape[1], mouse_pos[1]))
                g.draw.line(surf, (255, 0, 0), g.Vector2(mouse_pos[0], 0), g.Vector2(mouse_pos[0], source.shape[0]))
            for crop in crop_set:
                g.draw.rect(surf, (0, 255, 0), g.Rect(crop[0], crop[1], crop[2] - crop[0], crop[3] - crop[1]), 1)
            
            g.display.update()

        for crop in crop_set:
            c = source.copy()
            c = c[ crop[1]:crop[3], crop[0]:crop[2]]
            cv2.imwrite(os.path.join(output_dir, f'{hashlib.sha256(c.tobytes()).hexdigest()}.jpg'), c)
