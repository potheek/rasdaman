-- Testbed: result_type=set <marray <char, [5:10]>>
-- Testbed: result_elements=2

select a[(5:10).lo, 5:10]
from ImgCharA as a

