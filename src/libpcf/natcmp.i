/**
 * @file natcmp.i
 * @author Daniel Starke
 * @see natcmps.h
 * @date 2020-01-02
 * @version 2020-01-02
 * @internal This file is never used or compiled directly but only included.
 * @remarks Define CHAR_T to the character type before including this file.
 * @remarks Define INT_T to the character integer type before including this file.
 * @remarks See NATCMP_FUNC() for further notes.
 */


/**
 * Returns the same as strcmp() but compares number elements in the string as numbers and ignores
 * spaces.
 *
 * @param[in] lhs - left-hand statement
 * @param[in] rhs - right-hand statement
 * @return <0 if lhs < rhs; 0 if lhs == rhs, >0 if lhs > rhs
 * @remarks Define IS_DIGIT_FUNC for the digit detection function.
 * @remarks Define IS_ZERO_FUNC for the space detection function.
 * @remarks Define IS_SPACE_FUNC for the space detection function.
 * @remarks Optionally, define TO_UPPER_FUNC for the upper-case conversion function.
 */
int NATCMP_FUNC(const CHAR_T * lhs, const CHAR_T * rhs) {
	int iL, iR;
	INT_T cL, cR;
	int result;

	if (lhs == NULL && rhs == NULL) return 0;
	if (lhs == NULL) return -1;
	if (rhs == NULL) return 1;

	for (iL = 0, iR = 0, result = 0; ; iL++, iR++) {
		cL = (INT_T)lhs[iL];
		cR = (INT_T)rhs[iR];

		/* skip leading spaces */
		while (IS_SPACE_FUNC(cL) != 0) cL = (INT_T)lhs[++iL];
		while (IS_SPACE_FUNC(cR) != 0) cR = (INT_T)rhs[++iR];

		/* compare numbers */
		if (IS_DIGIT_FUNC(cL) != 0 && IS_DIGIT_FUNC(cR) != 0) {
			/* skip leading zeros */
			while (IS_ZERO_FUNC(cL) != 0) cL = (INT_T)lhs[++iL];
			while (IS_ZERO_FUNC(cR) != 0) cR = (INT_T)rhs[++iR];
			/* find longest number or number with largest value */
			for (;IS_DIGIT_FUNC(cL) != 0 || IS_DIGIT_FUNC(cR) != 0; cL = (INT_T)lhs[++iL], cR = (INT_T)rhs[++iR]) {
				if (IS_DIGIT_FUNC(cL) == 0) return -1;
				if (IS_DIGIT_FUNC(cR) == 0) return 1;
				/* remember first different digit */
				if (cL < cR) {
					if (result == 0) result = -1;
				} else if (cL > cR) {
					if (result == 0) result = 1;
				}
			}
			if (result != 0) return result;
		}

		if (cL == 0 && cR == 0) break;

#ifdef TO_UPPER_FUNC
		cL = (INT_T)TO_UPPER_FUNC(cL);
		cR = (INT_T)TO_UPPER_FUNC(cR);
#endif /* TO_UPPER_FUNC */

		if (cL < cR) return -1;
		if (cL > cR) return 1;
	}

	return 0;
}
