#!/usr/bin/env python3

from tkinter import *
import subprocess

proc = subprocess.Popen(['./sudoku', 'v'], stderr=subprocess.PIPE, universal_newlines=True)
lines = proc.stderr.readlines()

statuses = []
states = []
cur_state = 0

def empty_sudoku():
	return [[(0, '')] * 9 for _ in range(9)]

def copy_sudoku(src):
	return [[src[i][j] for j in range(9)] for i in range(9)]

def bolden_sudoku(src):
	return [[(src[i][j][0], 'bold') for j in range(9)] for i in range(9)]

def read_states(initial, lines, style=''):
	global states, statuses

	if len(lines) == 0:
		return lines

	new_state = copy_sudoku(initial)

	line = lines[0]
	statuses.append(line)

	spl = line.split()
	if spl[0] == 'INPUT':
		num = int(spl[1])
		[row,col] = [int(x) for x in spl[3].strip('()').split(',')]
		new_state[row][col] = (num, style)
		states.append(new_state)
		return read_states(new_state, lines[1:])
	elif spl[0] == 'GUESS':
		states.append(new_state)
		lines = read_states(new_state, lines[1:], style='italic')
		return read_states(initial, lines)
	elif spl[0] == 'BAD_GUESS':
		states.append(new_state)
		return lines[1:]
	elif spl[0] == 'SOLVING':
		initial = new_state = bolden_sudoku(new_state)
		states = []
		statuses = [line]

	states.append(new_state)
	return read_states(initial, lines[1:])

read_states(empty_sudoku(), lines)

def show_sudoku(sudoku):
	for i in range(9):
		for j in range(9):
			cells[i][j].set(str(sudoku[i][j]))

def show_current():
	if cur_state >= len(states):
		return

	show_sudoku(states[cur_state])
	status.set(statuses[cur_state])

def show_next():
	global cur_state
	cur_state = min(len(states) - 1, cur_state + 1)
	show_current()

def show_prev():
	global cur_state
	cur_state = max(0, cur_state - 1)
	show_current()

root = Tk()

status = StringVar()
status_label = Label(root, textvariable=status)
status_label.pack()

frame = Frame(root)

cells = [[StringVar() for _ in range(9)] for _ in range(9)]
labels = [[Label(frame, textvariable=cells[i][j]) for j in range(9)] for i in range(9)]

for i in range(9):
	for j in range(9):
		labels[i][j].grid(row=i, column=j)

frame.pack()

def show_sudoku(sudoku):
	for i, row in enumerate(sudoku):
		for j, (num, style) in enumerate(row):
			labels[i][j].config(font=('TkFixedFont 20 ' + style))
			cells[i][j].set(str(num) if num != 0 else ' ')

prev_button = Button(root, text='Prev', command=show_prev)
prev_button.pack()
next_button = Button(root, text='Next', command=show_next)
next_button.pack()

mainloop()
