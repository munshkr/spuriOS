int data[10];

int main(void) {
	int* p = (int*) 0x1;
	*p = 100;

	while (1);

	data[0] = 0x10;
	data[1] = data[0];

	// void* p = palloc();
	return 0;
}
