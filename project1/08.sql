select avg(level) from CatchedPokemon as cp where cp.owner_id = (select tr.id from Trainer as tr where tr.name = 'Red');