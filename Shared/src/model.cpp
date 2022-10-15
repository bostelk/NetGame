#include <vector>
#include <cassert>
#include <string>

// todo template specialization ???
struct net_field_t
{
	std::string name;
	size_t offset;
	size_t size_bytes;
	uint8_t* bytes;

	// hack.
	size_t num_elements;

	// for trivial types.
	void copy(uint8_t* src)
	{
		memcpy(bytes, src, size_bytes);
	}

	void alloc()
	{
		bytes = (uint8_t*)malloc(size_bytes);
		assert(bytes != nullptr);
		memset(bytes, 0, size_bytes);
	}

	void release()
	{
		free(bytes);
	}

	static bool equals(const net_field_t& left, const net_field_t& right)
	{
		// todo specialized comparison for floats.
		return memcmp(left.bytes, right.bytes, left.size_bytes) == 0;
	}

	// a - b = c
	static net_field_t diff(net_field_t& a, net_field_t& b)
	{
		net_field_t c = a;
		c.alloc();
		c.copy(a.bytes);

		int* arr_b = (int*)b.bytes;
		int* arr_c = (int*)c.bytes;
		for (int i = 0; i < a.num_elements; i++)
		{
			arr_c[i] = arr_c[i] - arr_b[i];
			if (arr_c[i] == 0)
			{
				// todo sparse array ?
				//assert(false);
			}
		}
		return c;
	}
};

struct player_state_t
{
	int position[3];

	player_state_t() :
		position()
	{

	}

	std::vector<net_field_t> get_net_fields()
	{
		std::vector<net_field_t> fields;
		net_field_t new_field;
		new_field.name = "position";
		new_field.offset = offsetof(player_state_t, position);
		new_field.size_bytes = sizeof(position);
		new_field.num_elements = new_field.size_bytes / sizeof(int);
		new_field.alloc();
		new_field.copy((uint8_t*)position);
		fields.push_back(new_field);
		return fields;
	}

	static bool equals(player_state_t& left, player_state_t& right)
	{
		auto left_fields = left.get_net_fields();
		auto right_fields = right.get_net_fields();
		assert(left_fields.size() == right_fields.size());
		for (int i = 0; i < left_fields.size(); i++)
		{
			bool equal = net_field_t::equals(left_fields[i], right_fields[i]);
			if (!equal)
			{
				return false;
			}
		}
		return true;
	}

	// a - b = c
	static std::vector<net_field_t> diff(player_state_t& a, player_state_t& b)
	{
		// c
		std::vector<net_field_t> fields;
		auto a_fields = a.get_net_fields();
		auto b_fields = b.get_net_fields();
		assert(a_fields.size() == b_fields.size());
		for (int i = 0; i < a_fields.size(); i++)
		{
			assert(a_fields[i].name == b_fields[i].name);
			bool equal = net_field_t::equals(a_fields[i], b_fields[i]);
			if (!equal)
			{
				net_field_t new_field = net_field_t::diff(a_fields[i], b_fields[i]);
				fields.push_back(new_field);
			}
		}
		return fields;
	}
};

struct snapshot_t
{
	player_state_t player1;

	snapshot_t() :
		player1({})
	{
	}

	void patch(std::vector<net_field_t> fields)
	{
		for (auto f : fields)
		{
			size_t offset = offsetof(snapshot_t, player1);
			size_t offset_to_field = offset + f.offset;
			uint8_t* base = (uint8_t*)this;
			uint8_t* field_ptr = base + offset_to_field;

			int* this_arr = (int*)field_ptr;
			int* f_arr = (int*)f.bytes;
			for (int i = 0; i < f.num_elements; i++)
			{
				this_arr[i] = this_arr[i] + f_arr[i];
			}
		}
	}

	static bool equals(snapshot_t& left, snapshot_t& right)
	{
		return player_state_t::equals(left.player1, right.player1);
	}

	// a - b = c
	static std::vector<net_field_t> diff(snapshot_t& a, snapshot_t& b)
	{
		std::vector<net_field_t> fields;
		auto diff = player_state_t::diff(a.player1, b.player1);
		for (auto f : diff)
		{
			fields.push_back(f);
		}
		return fields;
	}
};

int main()
{
	snapshot_t t0;
	t0.player1.position[0] = 3;
	t0.player1.position[1] = 3;
	t0.player1.position[2] = 3;

	snapshot_t t1;
	t1.player1.position[0] = 10;
	t1.player1.position[1] = 3;
	t1.player1.position[2] = 10;

	// find the difference between the second (t0) and first (t1) snapshot.
	auto diff = snapshot_t::diff(t1, t0);

	// add difference to the first snapshot (t0) to get the second (t1).
	snapshot_t client = t0;
	client.patch(diff);

	assert(snapshot_t::equals(t1, client));

	return 0;
}