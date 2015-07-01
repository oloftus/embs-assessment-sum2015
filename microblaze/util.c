int roundUpIntDiv(int num, int denom) {

	int i = 0;
	while (num > 0) {
		num -= denom;
		i++;
	}

	return i;
}
