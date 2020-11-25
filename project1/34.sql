select name, level, nickname from Gym as g1 inner join CatchedPokemon as cp1 on g1.leader_id = cp1.owner_id inner join Pokemon as p1 on cp1.pid=p1.id where nickname like 'A%' order by name desc;
