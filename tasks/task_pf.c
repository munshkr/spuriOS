int main() {
	//*(int*)(0x600000) = 0xff;	// unknown segment
	*(int*)(0x1200) = 0xff;		// kernel segment
	return 0;
}
