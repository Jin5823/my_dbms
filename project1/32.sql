select name from Pokemon as p1 left outer join CatchedPokemon as cp1 on p1.id = cp1.pid where owner_id is null order by name asc;
