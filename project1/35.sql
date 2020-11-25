select name from Evolution as e1 inner join Pokemon as p1 on e1.before_id = p1.id where after_id < before_id order by name asc;
