var i = 10

while i != 0: {
    i = i - 1
}

var lsi = [0, 1, 2, 3]
i = 0

while i < 3: {
    i = i + 1
    if lsi[i] == 2:
        continue
    elif lsi[i] == 3:
        break
}

i = 0

while i < 3: {
    break
}

while i < 3: {
    i = i + 1
    continue
}

for i in 1...10: { }
if i != 10:
    print("for i in 1...10 did not end with i at 10.\n")

for i in 10...1: { }
if i != 1:
    print("for i in 10...1 did not end with i at 1.\n")

var intlist = [0, 0, 0, 0, 0, 0]
for i in 1...5 by 2:
    intlist[i] = 1

if intlist != [0, 1, 0, 1, 0, 1]:
    print("for i in 1...5 by 2 not incrementing right.\n")
