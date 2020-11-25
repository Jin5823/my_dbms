select sum(level) from CatchedPokemon as cp1 inner join Trainer as t1 on cp1.owner_id = t1.id where name = 'Matis';
