from decimal import Decimal
import os

def transform_path(path_str: str, delta_x: Decimal, delta_y: Decimal) -> str:
    new_tokens = []
    coord_index = 0

    for token in path_str.split():
        try:
            number = Decimal(token)

            if coord_index % 2 == 0:
                number += delta_x
            else:
                number += delta_y

            new_tokens.append(str(number))
            coord_index += 1
        except:
            coord_index = 0
            new_tokens.append(token)

    return " ".join(new_tokens)


os.chdir(os.path.dirname(__file__))

with open("path.txt", "r", encoding="utf-8") as input:
    path_str = input.read()

with open("output.txt", "w", encoding="utf-8") as output:
    output.write('<x:String x:Key="">');
    output.write(transform_path(path_str, Decimal(0), Decimal(20)))
    output.write('</x:String>');
