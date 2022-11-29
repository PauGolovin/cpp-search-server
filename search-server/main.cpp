// Решите загадку: Сколько чисел от 1 до 1000 содержат как минимум одну цифру 3?

#include <iostream>
#include <string>

using namespace std;

int main() 
{
	int count = 0;

	for (int i = 1; i <= 1000; ++i)
	{
		string itos = to_string(i);
		for(char& ch : itos)
			if (ch == '3')
			{
				++count;
				break;
			}
	}

	cout << count << endl;
}

// Напишите ответ здесь:

271

// Закомитьте изменения и отправьте их в свой репозиторий.
