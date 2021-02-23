def area(n, inputs):
    area = 0
    for i in range(1,n):
        x1 = inputs[i-1][0]
        y1 = inputs[i-1][1]
        x2 = inputs[i][0]
        y2 = inputs[i][1]
        if y1*y2 >= 0:
            area += abs((x2 - x1)*(y1 + y2)/2)
        else:
            area += 1/2*abs((x2 - x1)/(y2 - y1))*(y1*y1 + y2*y2)
    return area

string = ""
for i in range(8):
    name = "A1_Test" + str(i+1) + ".in"
    file = open(name, "r")
    inputs = []
    n = int(file.readline())
    for j in range(n):
        inputs.append(tuple(map(int, file.readline().split())))
    string += "A1_Test" + str(i+1)+".in: " + str(area(n, inputs)) + "\n"
    file.close()

file = open("A1_Test_out.out","w")
file.write(string)
file.close()
