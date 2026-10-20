file = "test_2nd.c"

clear = open("propability.csv", "w")
clear.write("")

x = open(file, "r")
print("File opened successfully")
propability = 0.0
counted_lines = 0

for line in x:
   # if counted_lines < 5:
        if line.find("->") != -1:
            counted_lines += 1
            propability += 0.3
            print("pass")
        elif line.find("Node") != -1 or line.find("node") != -1:
            counted_lines += 1
            propability += 0.2
            print("2nd pass")
        else:
            propability -= 0.1
            counted_lines +=1  

        
print("lines = "+str(counted_lines))        

with open("propability.csv", "a") as f:
    if (counted_lines == 0):
        f.write(f"0\n")
    else:
        f.write(f"{propability/2}\n")