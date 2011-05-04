int data[10];

extern unsigned int getpid(void);

int main(void) {
	unsigned int id = getpid();
	while (1);
	return id;
}
